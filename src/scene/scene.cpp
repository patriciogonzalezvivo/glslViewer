#include "scene.h"
#include "window.h"

#include <unistd.h>

#include "tools/fs.h"
#include "tools/geom.h"
#include "tools/text.h"
#include "tools/shapes.h"

#include "default_scene_shaders.h"

Scene::Scene(): 
    // Camera.
    m_culling(NONE), m_lat(180.0), m_lon(0.0),
    // Light
    m_light_vbo(nullptr), m_dynamicShadows(false),
    // CubeMap
    m_cubemap_vbo(nullptr), m_cubemap(nullptr), m_cubemap_skybox(nullptr), m_cubemap_draw(false), 
    // UI
    m_grid_vbo(nullptr), m_axis_vbo(nullptr) {
}

Scene::~Scene(){
    clear();
}

void Scene::clear() {
    for (unsigned int i = 0; i < m_models.size(); i++) 
        delete m_models[i];

    m_models.clear();

    if (m_light_vbo) {
        delete m_light_vbo;
        m_light_vbo = nullptr;
    }

    if (m_cubemap) {
        delete m_cubemap;
        m_cubemap = nullptr;
    }

    if (m_cubemap_vbo) {
        delete m_cubemap_vbo;
        m_cubemap_vbo = nullptr;
    }

    if (m_grid_vbo) {
        delete m_grid_vbo;
        m_grid_vbo = nullptr;
    }

    if (m_axis_vbo) {
        delete m_axis_vbo;
        m_axis_vbo = nullptr;
    }
}

void Scene::setup(CommandList &_commands, Uniforms &_uniforms) {
    // Set up camera
    m_camera.setViewport(getWindowWidth(), getWindowHeight());
    m_camera.setPosition(glm::vec3(0.0,0.0,-3.));

    // Setup light
    m_light.setPosition(glm::vec3(0.0,100.0,100.0));

    // ADD COMMANDS
    // ----------------------------------------- 
    _commands.push_back(Command("culling", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 1) {
            if (getCulling() == NONE) {
                std::cout << "none" << std::endl;
            }
            else if (getCulling() == FRONT) {
                std::cout << "front" << std::endl;
            }
            else if (getCulling() == BACK) {
                std::cout << "back" << std::endl;
            }
            else if (getCulling() == BOTH) {
                std::cout << "both" << std::endl;
            }
            return true;
        }
        else if (values.size() == 2) {
            if (values[1] == "none") {
                setCulling(NONE);
            }
            else if (values[1] == "front") {
                setCulling(FRONT);
            }
            else if (values[1] == "back") {
                setCulling(BACK);
            }
            else if (values[1] == "both") {
                setCulling(BOTH);
            }
            return true;
        }

        return false;
    },
    "culling[,<none|front|back|both>]   get or set the culling modes"));

    _commands.push_back(Command("camera_distance", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 2) {
            m_camera.setDistance(toFloat(values[1]));
            return true;
        }
        else {
            std::cout << m_camera.getDistance() << std::endl;
            return true;
        }
        return false;
    },
    "camera_distance[,<dist>]       get or set the camera distance to the target."));

    _commands.push_back(Command("camera_fov", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 2) {
            m_camera.setFOV(toFloat(values[1]));
            return true;
        }
        else {
            std::cout << m_camera.getFOV() << std::endl;
            return true;
        }
        return false;
    },
    "camera_fov[,<field_of_view>]   get or set the camera field of view."));

    _commands.push_back(Command("camera_position", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 4) {
            m_camera.setPosition(glm::vec3(toFloat(values[1]),toFloat(values[2]),toFloat(values[3])));
            m_camera.lookAt(m_camera.getTarget());
            return true;
        }
        else {
            glm::vec3 pos = m_camera.getPosition();
            std::cout << pos.x << ',' << pos.y << ',' << pos.z << std::endl;
            return true;
        }
        return false;
    },
    "camera_position[,<x>,<y>,<z>]  get or set the camera position."));

    _commands.push_back(Command("camera_exposure", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 4) {
            m_camera.setExposure(toFloat(values[1]),toFloat(values[2]),toFloat(values[3]));
            return true;
        }
        else {
            std::cout << m_camera.getExposure() << std::endl;
            return true;
        }
        return false;
    },
    "camera_exposure[,<aperture>,<shutterSpeed>,<sensitivity>]  get or set the camera exposure. Defaults: 16, 1/125s, 100 ISO"));

    _commands.push_back(Command("light_position", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 4) {
            m_light.setPosition(glm::vec3(toFloat(values[1]),toFloat(values[2]),toFloat(values[3])));
            return true;
        }
        else {
            glm::vec3 pos = m_light.getPosition();
            std::cout << pos.x << ',' << pos.y << ',' << pos.z << std::endl;
            return true;
        }
        return false;
    },
    "light_position[,<x>,<y>,<z>]  get or set the light position."));

    _commands.push_back(Command("light_color", [&](const std::string& _line){ 
         std::vector<std::string> values = split(_line,',');
        if (values.size() == 4) {
            m_light.color = glm::vec3(toFloat(values[1]),toFloat(values[2]),toFloat(values[3]));
            m_light.bChange = true;
            return true;
        }
        else {
            glm::vec3 color = m_light.color;
            std::cout << color.x << ',' << color.y << ',' << color.z << std::endl;
            return true;
        }
        return false;
    },
    "light_color[,<r>,<g>,<b>]      get or set the light color."));
    
    _commands.push_back(Command("dynamic_shadows", [&](const std::string& _line){ 
        if (_line == "dynamic_shadows") {
            std::string rta = getDynamicShadows() ? "on" : "off";
            std::cout <<  rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = split(_line,',');
            if (values.size() == 2) {
                setDynamicShadows( (values[1] == "on") );
            }
        }
        return false;
    },
    "dynamic_shadows[on|off]        get or set dynamic shadows"));

    _commands.push_back(Command("skybox_ground", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 4) {
            m_skybox.groundAlbedo = glm::vec3(toFloat(values[1]),toFloat(values[2]),toFloat(values[3]));
            setCubeMap(&m_skybox);
            return true;
        }
        else {
            std::cout << m_skybox.groundAlbedo.x << ',' << m_skybox.groundAlbedo.y << ',' << m_skybox.groundAlbedo.z << std::endl;
            return true;
        }
        return false;
    },
    "skybox_ground[,<r>,<g>,<b>]      get or set the ground color of the skybox."));

    _commands.push_back(Command("skybox_elevation", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 2) {
            m_skybox.elevation = toFloat(values[1]);
            setCubeMap(&m_skybox);
            return true;
        }
        else {
            std::cout << m_skybox.elevation << std::endl;
            return true;
        }
        return false;
    },
    "skybox_elevation[,<sun_elevation>]  get or set the sun elevation (in rads) of the skybox."));

    _commands.push_back(Command("skybox_azimuth", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 2) {
            m_skybox.azimuth = toFloat(values[1]);
            setCubeMap(&m_skybox);
            return true;
        }
        else {
            std::cout << m_skybox.azimuth << std::endl;
            return true;
        }
        return false;
    },
    "skybox_azimuth[,<sun_azimuth>]  get or set the sun azimuth (in rads) of the skybox."));

    _commands.push_back(Command("skybox_turbidity", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 2) {
            m_skybox.turbidity = toFloat(values[1]);
            setCubeMap(&m_skybox);
            return true;
        }
        else {
            std::cout << m_skybox.turbidity << std::endl;
            return true;
        }
        return false;
    },
    "skybox_turbidity[,<sky_turbidty>]  get or set the sky turbidity of the m_skybox."));

    _commands.push_back(Command("skybox", [&](const std::string& _line){
        if (_line == "skybox") {
            std::string rta = getCubeMapVisible() ? "on" : "off";
            std::cout << rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = split(_line,',');
            if (values.size() == 2) {
                setCubeMap(&m_skybox);
                setCubeMapVisible( values[1] == "on" );
            }
        }
        return false;
    },
    "skybox[,on|off]       show/hide skybox"));

    _commands.push_back(Command("cubemap", [&](const std::string& _line){
        if (_line == "cubemap") {
            std::string rta = getCubeMapVisible() ? "on" : "off";
            std::cout << rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = split(_line,',');
            if (values.size() == 2) {
                setCubeMapVisible( values[1] == "on" );
            }
        }
        return false;
    },
    "cubemap[,on|off]       show/hide cubemap"));
    
    _commands.push_back(Command("model_position", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 4) {
            m_origin.setPosition( glm::vec3(toFloat(values[1]),toFloat(values[2]),toFloat(values[3])) );
            m_light.bChange = true;
            return true;
        }
        else {
            glm::vec3 pos = m_origin.getPosition();
            std::cout << pos.x << ',' << pos.y << ',' << pos.z << std::endl;
            return true;
        }
        return false;
    },
    "model_position[,<x>,<y>,<z>]  get or set the model position."));

    _commands.push_back(Command("wait", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 2) {
            usleep( toFloat(values[1])*1000000 ); 
        }
        return false;
    },
    "wait,<seconds>                 wait for X <seconds> before excecuting another command."));

    // LIGHT UNIFORMS
    //
    _uniforms.functions["u_light"] = UniformFunction("vec3", [this](Shader& _shader) {
        _shader.setUniform("u_light", m_light.getPosition());
    },
    [this]() { return toString(m_light.getPosition(), ','); });

    _uniforms.functions["u_lightColor"] = UniformFunction("vec3", [this](Shader& _shader) {
        _shader.setUniform("u_lightColor", m_light.color);
    },
    [this]() { return toString(m_light.color, ','); });

    _uniforms.functions["u_lightMatrix"] = UniformFunction("mat4", [this](Shader& _shader) {
        _shader.setUniform("u_lightMatrix", m_light.getBiasMVPMatrix() );
    });

    _uniforms.functions["u_ligthShadowMap"] = UniformFunction("sampler2D", [this](Shader& _shader) {
        if (m_light_depthfbo.getDepthTextureId()) {
            _shader.setUniformDepthTexture("u_ligthShadowMap", &m_light_depthfbo);
        }
    });

    // IBL UNIFORM
    _uniforms.functions["u_cubeMap"] = UniformFunction("samplerCube", [this](Shader& _shader) {
        if (m_cubemap) {
            _shader.setUniformTextureCube("u_cubeMap", (TextureCube*)m_cubemap);
        }
    });

    _uniforms.functions["u_SH"] = UniformFunction("vec3", [this](Shader& _shader) {
        if (m_cubemap) {
            _shader.setUniform("u_SH", m_cubemap->SH, 9);
        }
    });

    _uniforms.functions["u_iblLuminance"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_iblLuminance", 30000.0f * m_camera.getExposure());
    },
    [this]() { return toString(30000.0f * m_camera.getExposure()); });
    
    // CAMERA UNIFORMS
    //
    _uniforms.functions["u_camera"] = UniformFunction("vec3", [this](Shader& _shader) {
        _shader.setUniform("u_camera", -m_camera.getPosition());
    },
    [this]() { return toString(-m_camera.getPosition(), ','); });

    _uniforms.functions["u_cameraDistance"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraDistance", m_camera.getDistance());
    },
    [this]() { return toString(m_camera.getDistance()); });

    _uniforms.functions["u_cameraNearClip"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraNearClip", m_camera.getNearClip());
    },
    [this]() { return toString(m_camera.getNearClip()); });

    _uniforms.functions["u_cameraFarClip"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraFarClip", m_camera.getFarClip());
    },
    [this]() { return toString(m_camera.getFarClip()); });

    _uniforms.functions["u_cameraEv100"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraEv100", m_camera.getEv100());
    },
    [this]() { return toString(m_camera.getEv100()); });

    _uniforms.functions["u_cameraExposure"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraExposure", m_camera.getExposure());
    },
    [this]() { return toString(m_camera.getExposure()); });

    _uniforms.functions["u_cameraAperture"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraAperture", m_camera.getAperture());
    },
    [this]() { return toString(m_camera.getAperture()); });

    _uniforms.functions["u_cameraShutterSpeed"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraShutterSpeed", m_camera.getShutterSpeed());
    },
    [this]() { return toString(m_camera.getShutterSpeed()); });

    _uniforms.functions["u_cameraSensitivity"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraSensitivity", m_camera.getSensitivity());
    },
    [this]() { return toString(m_camera.getSensitivity()); });
    
    // MATRIX UNIFORMS
    _uniforms.functions["u_model"] = UniformFunction("vec3", [this](Shader& _shader) {
        _shader.setUniform("u_model", m_origin.getPosition());
    },
    [this]() { return toString(m_origin.getPosition(), ','); });

    _uniforms.functions["u_normalMatrix"] = UniformFunction("mat3", [this](Shader& _shader) {
        _shader.setUniform("u_normalMatrix", m_camera.getNormalMatrix());
    });

    _uniforms.functions["u_modelMatrix"] = UniformFunction("mat4", [this](Shader& _shader) {
        _shader.setUniform("u_modelMatrix", m_origin.getTransformMatrix() );
    });

    _uniforms.functions["u_viewMatrix"] = UniformFunction("mat4", [this](Shader& _shader) {
        _shader.setUniform("u_viewMatrix", m_camera.getViewMatrix());
    });

    _uniforms.functions["u_projectionMatrix"] = UniformFunction("mat4", [this](Shader& _shader) {
        _shader.setUniform("u_projectionMatrix", m_camera.getProjectionMatrix());
    });
}

void Scene::addDefine(const std::string &_define, const std::string &_value) {
    for (unsigned int i = 0; i < m_models.size(); i++) {
        m_models[i]->addDefine(_define, _value);
    }
}

void Scene::delDefine(const std::string &_define) {
    for (unsigned int i = 0; i < m_models.size(); i++) {
        m_models[i]->delDefine(_define);
    }
}

void Scene::printDefines() {
    for (unsigned int i = 0; i < m_models.size(); i++) {
        std::cout << std::endl;
        std::cout << m_models[i]-> getName() << std::endl;
        std::cout << " -------------- " << std::endl;
        m_models[i]->printDefines();
    }
}

bool Scene::loadGeometry( WatchFileList &filenames, int _index, bool _verbose) {

    std::string filename = filenames[_index].path;

    // If the geometry is a PLY it's easy because is only one mesh
    if ( haveExt(filename,"ply") || haveExt(filename,"PLY") ) {
        Mesh mesh;
        bool ret = mesh.load(filename);

        if (!ret) {
            std::cerr << "Failed to load " << filename.c_str() << std::endl;
            return false;
        }

        m_models.push_back( new Model("", mesh ) );
    }

    // If it's a OBJ could be more complicated because they can contain several meshes
    else if ( haveExt(filename,"obj") || haveExt(filename,"OBJ") ) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;

        std::string warn;
        std::string err;
        std::string base_dir = getBaseDir(filename.c_str());
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str(), base_dir.c_str());

        if (!warn.empty()) {
            std::cout << "WARN: " << warn << std::endl;
        }
        if (!err.empty()) {
            std::cerr << err << std::endl;
        }
        if (!ret) {
            std::cerr << "Failed to load " << filename.c_str() << std::endl;
            return false;
        }

        // Append `default` material
        // materials.push_back(tinyobj::material_t());

        if (_verbose) {
            std::cerr << "Loading " << filename.c_str() << std::endl;
            printf("    Total ertices   = %d\n", (int)(attrib.vertices.size()) / 3);
            printf("    Total normals   = %d\n", (int)(attrib.normals.size()) / 3);
            printf("    Total texcoords = %d\n", (int)(attrib.texcoords.size()) / 2);
            printf("    Total materials = %d\n", (int)materials.size());
            printf("    Total shapes    = %d\n", (int)shapes.size());

            // std::cerr << "Materials: " << filename.c_str() << std::endl;
            // for (size_t i = 0; i < materials.size(); i++) {

            //     printf("material[%d].diffuse_texname = %s\n", int(i), materials[i].diffuse_texname.c_str());
            // }

            std::cerr << "Shapes: " << std::endl;
        }

        // TODO Load diffuse textures
        for (size_t s = 0; s < shapes.size(); s++) {

            std::string name = shapes[s].name;
            if (name.empty())
                name = toString(s);

            if (_verbose)
                std::cerr << name << std::endl;

            // Check for smoothing group and compute smoothing normals
            std::map<int, glm::vec3> smoothVertexNormals;
            if (hasSmoothingGroup(shapes[s]) > 0) {
                std::cout << "  Compute smoothingNormal" << std::endl;
                computeSmoothingNormals(attrib, shapes[s], smoothVertexNormals);
            }

            Mesh mesh;
            mesh.setDrawMode(GL_TRIANGLES);

            INDEX_TYPE index = 0;
            std::vector<glm::vec4> colors;
            std::vector<glm::vec3> normals;
            std::vector<glm::vec2> uvs;

            for (size_t f = 0; f < shapes[s].mesh.indices.size() / 3; f++) {
                tinyobj::index_t idx0 = shapes[s].mesh.indices[3 * f + 0];
                tinyobj::index_t idx1 = shapes[s].mesh.indices[3 * f + 1];
                tinyobj::index_t idx2 = shapes[s].mesh.indices[3 * f + 2];

                int current_material_id = shapes[s].mesh.material_ids[f];

                if ((current_material_id >= 0) && (current_material_id < static_cast<int>(materials.size()))) {
                    // Extract vertex color
                    glm::vec4 c = glm::vec4(1.0, 1.0, 1.0, 1.0);
                    for (size_t i = 0; i < 3; i++) {
                        c[i] = materials[current_material_id].diffuse[i];
                    }
                    std::cout << "    Diffuse color: " << c.x << "," << c.y << "," << c.z << "," << c.w << std::endl;
                    colors.push_back(c);
                    colors.push_back(c);
                    colors.push_back(c);
                }

                glm::vec3 v[3];
                for (int k = 0; k < 3; k++) {
                    int f0 = idx0.vertex_index;
                    int f1 = idx1.vertex_index;
                    int f2 = idx2.vertex_index;
                    assert(f0 >= 0);
                    assert(f1 >= 0);
                    assert(f2 >= 0);

                    v[0][k] = attrib.vertices[3 * f0 + k];
                    v[1][k] = attrib.vertices[3 * f1 + k];
                    v[2][k] = attrib.vertices[3 * f2 + k];
                }
                
                bool invalid_normal_index = false;
                if (attrib.normals.size() > 0) {
                    int nf0 = idx0.normal_index;
                    int nf1 = idx1.normal_index;
                    int nf2 = idx2.normal_index;

                    if ((nf0 < 0) || (nf1 < 0) || (nf2 < 0)) {
                            // normal index is missing from this face.
                            invalid_normal_index = true;
                    } 
                    else {
                        glm::vec3 n[3];
                        for (int k = 0; k < 3; k++) {
                            assert(size_t(3 * nf0 + k) < attrib.normals.size());
                            assert(size_t(3 * nf1 + k) < attrib.normals.size());
                            assert(size_t(3 * nf2 + k) < attrib.normals.size());
                            n[0][k] = attrib.normals[3 * nf0 + k];
                            n[1][k] = attrib.normals[3 * nf1 + k];
                            n[2][k] = attrib.normals[3 * nf2 + k];
                        }
                        normals.push_back(n[0]);
                        normals.push_back(n[1]);
                        normals.push_back(n[2]);
                    }
                }
                else {
                    invalid_normal_index = true;
                }

                if (invalid_normal_index && !smoothVertexNormals.empty()) {
                    // Use smoothing normals
                    int f0 = idx0.vertex_index;
                    int f1 = idx1.vertex_index;
                    int f2 = idx2.vertex_index;

                    if (f0 >= 0 && f1 >= 0 && f2 >= 0) {
                        normals.push_back( smoothVertexNormals[f0] );
                        normals.push_back( smoothVertexNormals[f1] );
                        normals.push_back( smoothVertexNormals[f2] );
                        invalid_normal_index = false;
                    }
                }

                if (attrib.texcoords.size() > 0) {
                    if ((idx0.texcoord_index >= 0) && (idx1.texcoord_index >= 0) && (idx2.texcoord_index >= 0)) {
                        assert(attrib.texcoords.size() > size_t(2 * idx0.texcoord_index + 1));
                        assert(attrib.texcoords.size() > size_t(2 * idx1.texcoord_index + 1));
                        assert(attrib.texcoords.size() > size_t(2 * idx2.texcoord_index + 1));

                        // Flip Y coord.
                        uvs.push_back( glm::vec2(attrib.texcoords[2 * idx0.texcoord_index], 1.0f - attrib.texcoords[2 * idx0.texcoord_index + 1]) );
                        uvs.push_back( glm::vec2(attrib.texcoords[2 * idx1.texcoord_index], 1.0f - attrib.texcoords[2 * idx1.texcoord_index + 1]) );
                        uvs.push_back( glm::vec2(attrib.texcoords[2 * idx2.texcoord_index], 1.0f - attrib.texcoords[2 * idx2.texcoord_index + 1]) );
                    }
                }

                for (int k = 0; k < 3; k++) {
                    mesh.addVertex( v[k] );
                    mesh.addIndex(index);
                    index++;
                }
            }

            if (colors.size() == mesh.getVertices().size())
                mesh.addColors(colors);

            if (normals.size() == mesh.getVertices().size())
                mesh.addNormals(normals);

            if (uvs.size() == mesh.getVertices().size())
                mesh.addTexCoords(uvs);

            if (_verbose) {
                std::cout << "    vertices = " << mesh.getVertices().size() << std::endl;
                std::cout << "    colors   = " << mesh.getColors().size() << std::endl;
                std::cout << "    normals  = " << mesh.getNormals().size() << std::endl;
                std::cout << "    uvs      = " << mesh.getTexCoords().size() << std::endl;
            }

            if ( !mesh.hasNormals() ) {
                if (mesh.computeNormals())
                    if (_verbose)
                        std::cout << "    . Compute normals" << std::endl;
            }

            if ( mesh.computeTangents() )
                if (_verbose)
                    std::cout << "    . Compute tangents" << std::endl;

            m_models.push_back( new Model(name, mesh) );
        }
    }

    // Calculate the total area
    glm::vec3 min_v;
    glm::vec3 max_v;
    for (unsigned int i = 0; i < m_models.size(); i++) {
        expandBoundingBox( m_models[i]->getMinBoundingBox(), min_v, max_v);
        expandBoundingBox( m_models[i]->getMaxBoundingBox(), min_v, max_v);
    }
    m_area = glm::min(glm::length(min_v), glm::length(max_v));
    glm::vec3 centroid = (min_v + max_v) / 2.0f;
    m_origin.setPosition( -centroid );

    // set the right distance to the camera
    m_camera.setDistance( m_area * 2.0 );

    return true;
}

bool Scene::loadShaders(const std::string &_fragmentShader, const std::string &_vertexShader, bool _verbose) {

    bool rta = true;
    for (unsigned int i = 0; i < m_models.size(); i++) {
        rta += m_models[i]->loadShader( _fragmentShader, 
                                        _vertexShader, 
                                        _verbose);
    }
    return rta;
}

bool Scene::haveChange() const {
    return  m_light.bChange ||
            m_camera.bChange ||
            m_origin.bChange;
}

void Scene::unflagChange() { 
    m_light.bChange = false;
    m_camera.bChange = false;
    m_origin.bChange = false;
}

void Scene::render(Uniforms &_uniforms) {
    // Begining of DEPTH for 3D 
    glEnable(GL_DEPTH_TEST);

    if (m_culling != 0) {
        glEnable(GL_CULL_FACE);

        if (m_culling == 1) {
            glCullFace(GL_FRONT);
        }
        else if (m_culling == 2) {
            glCullFace(GL_BACK);
        }
        else if (m_culling == 3) {
            glCullFace(GL_FRONT_AND_BACK);
        }
    }

    if (m_camera.bChange || m_origin.bChange)
        m_mvp = m_camera.getProjectionViewMatrix() * m_origin.getTransformMatrix(); 
    
    for (unsigned int i = 0; i < m_models.size(); i++) {
        m_models[i]->draw(_uniforms, m_mvp);
    }

    glDisable(GL_DEPTH_TEST);

    if (m_culling != 0) {
        glDisable(GL_CULL_FACE);
    }
}

void Scene::renderShadowMap(Uniforms &_uniforms) {
    if ( m_origin.bChange || m_light.bChange ||  m_dynamicShadows ) {

        // Temporally move the MVP matrix from the view of the light 
        glm::mat4 mvp = m_light.getMVPMatrix( m_origin.getTransformMatrix(), m_area );

        if (m_light_depthfbo.getDepthTextureId() == 0) {
#ifdef PLATFORM_RPI
            m_light_depthfbo.allocate(512, 512, COLOR_DEPTH_TEXTURES);
#else
            m_light_depthfbo.allocate(1024, 1024, COLOR_DEPTH_TEXTURES);
#endif
        }

        m_light_depthfbo.bind();

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        // glCullFace(GL_FRONT);

        for (unsigned int i = 0; i < m_models.size(); i++) {
            m_models[i]->draw(_uniforms, mvp);
        }

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        m_light_depthfbo.unbind();
    }
}

void Scene::renderCubeMap() {
    // If there is a skybox and it had changes re generate
    if (m_cubemap_skybox) {
        if (m_cubemap_skybox->change) {
            if (!m_cubemap) {
                m_cubemap = new TextureCube();
            }
            m_cubemap->generate(m_cubemap_skybox);
            m_cubemap_skybox->change = false;
        }
    }

    if (m_cubemap && m_cubemap_draw) {

        if (!m_cubemap_vbo) {
            m_cubemap_vbo = cube(1.0f).getVbo();
            m_cubemap_shader.load(cube_frag, cube_vert, false);
        }

        m_cubemap_shader.use();

        m_cubemap_shader.setUniform("u_modelViewProjectionMatrix", m_camera.getProjectionMatrix() * glm::toMat4(m_camera.getOrientationQuat()) );
        m_cubemap_shader.setUniformTextureCube( "u_cubeMap", m_cubemap, 0 );

        m_cubemap_vbo->render( &m_cubemap_shader );
    }
}

void Scene::renderDebug() {
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    if (!m_wireframe3D_shader.isLoaded())
        m_wireframe3D_shader.load(wireframe3D_frag, wireframe3D_vert, false);

    // Draw Bounding boxes
    {
        glLineWidth(3.0f);
        m_wireframe3D_shader.use();
        m_wireframe3D_shader.setUniform("u_color", glm::vec4(1.0,1.0,0.0,1.0));
        m_wireframe3D_shader.setUniform("u_modelViewProjectionMatrix", m_mvp );
        for (unsigned int i = 0; i < m_models.size(); i++) {
            m_models[i]->getBboxVbo()->render( &m_wireframe3D_shader );
        }
    }
    
    // Axis
    if (m_axis_vbo == nullptr)
        m_axis_vbo = axis(m_camera.getFarClip()).getVbo();

    glLineWidth(2.0f);
    m_wireframe3D_shader.use();
    m_wireframe3D_shader.setUniform("u_color", glm::vec4(0.75));
    m_wireframe3D_shader.setUniform("u_modelViewProjectionMatrix", m_mvp );
    m_axis_vbo->render( &m_wireframe3D_shader );
    
    // Grid
    if (m_grid_vbo == nullptr)
        m_grid_vbo = grid(m_camera.getFarClip(), m_camera.getFarClip() / 20.0).getVbo();
    glLineWidth(1.0f);
    m_wireframe3D_shader.use();
    m_wireframe3D_shader.setUniform("u_color", glm::vec4(0.5));
    m_wireframe3D_shader.setUniform("u_modelViewProjectionMatrix", m_mvp );
    m_grid_vbo->render( &m_wireframe3D_shader );

    // Light
    if (!m_light_shader.isLoaded())
        m_light_shader.load(light_frag, light_vert, false);

    if (m_light_vbo == nullptr)
        m_light_vbo = rect(0.0,0.0,0.0,0.0).getVbo();
        // m_light_vbo = rect(0.0,0.0,1.0,1.0).getVbo();

    m_light_shader.use();
    m_light_shader.setUniform("u_scale", 24, 24);
    m_light_shader.setUniform("u_translate", m_light.getPosition());
    m_light_shader.setUniform("u_color", glm::vec4(m_light.color, 1.0));
    m_light_shader.setUniform("u_viewMatrix", m_camera.getViewMatrix());
    m_light_shader.setUniform("u_modelViewProjectionMatrix", m_mvp );

    m_light_vbo->render( &m_light_shader );

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
}

void Scene::onMouseDrag(float _x, float _y, int _button) {
    if (_button == 1) {
        // Left-button drag is used to rotate geometry.
        float dist = m_camera.getDistance();

        float vel_x = getMouseVelX();
        float vel_y = getMouseVelY();

        if (fabs(vel_x) < 50.0 && fabs(vel_y) < 50.0) {
            m_lat -= vel_x;
            m_lon -= vel_y * 0.5;
            m_camera.orbit(m_lat, m_lon, dist);
            m_camera.lookAt(glm::vec3(0.0));
        }
    } 
    else {
        // Right-button drag is used to zoom geometry.
        float dist = m_camera.getDistance();
        dist += (-.008f * getMouseVelY());
        if (dist > 0.0f) {
            m_camera.setDistance( dist );
        }
    }
}

void Scene::onViewportResize(int _newWidth, int _newHeight) {
    m_camera.setViewport(_newWidth, _newHeight);
}