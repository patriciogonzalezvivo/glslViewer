
#include "scene.h"
#include "window.h"

#ifndef PLATFORM_WINDOWS
#include <unistd.h>
#endif

#include <sys/stat.h>

#include "io/fs.h"
#include "tools/geom.h"
#include "tools/text.h"
#include "tools/shapes.h"

#include "shaders/light_ui.h"
#include "shaders/cubemap.h"
#include "shaders/wireframe3D.h"

#include "../io/ply.h"
#include "../io/obj.h"
#include "../io/gltf.h"

Scene::Scene(): 
    // Debug State
    showGrid(false), showAxis(false), showBBoxes(false), showCubebox(false), 
    // Camera.
    m_blend(ALPHA),
    m_culling(NONE),
    m_depth_test(true),
    // Light
    m_lightUI_vbo(nullptr), m_dynamicShadows(false),
    // Background
    m_background_vbo(nullptr), m_background(false), 
    // CubeMap
    m_cubemap_vbo(nullptr), m_cubemap_skybox(nullptr),
    // Floor
    m_floor_vbo(nullptr), m_floor_height(0.0), m_floor_subd_target(-1), m_floor_subd(-1), 
    // UI
    m_grid_vbo(nullptr), m_axis_vbo(nullptr) 
    {
}

Scene::~Scene(){
    clear();
}

void Scene::clear() {
    for (unsigned int i = 0; i < m_models.size(); i++) 
        delete m_models[i];

    m_models.clear();

    if (m_lightUI_vbo) {
        delete m_lightUI_vbo;
        m_lightUI_vbo = nullptr;
    }

    if (!m_background_vbo) {
        delete m_background_vbo;
        m_background_vbo = nullptr;
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

void Scene::setup(CommandList& _commands, Uniforms& _uniforms) {
    // ADD COMMANDS
    // ----------------------------------------- 
    _commands.push_back(Command("model", [&](const std::string& _line){ 
        if (_line == "models") {
            for (unsigned int i = 0; i < m_models.size(); i++)
                std::cout << m_models[i]->getName() << std::endl;
            return true;
        }
        else {
            std::vector<std::string> values = split(_line,',');
            if (values.size() == 2 && values[0] == "model")
                for (unsigned int i = 0; i < m_models.size(); i++)
                    if (m_models[i]->getName() == values[1]) {
                        m_models[i]->printVboInfo();
                        std::cout << "Defines:" << std::endl;
                        m_models[i]->printDefines();
                        return true;
                    }
        }

        return false;
    },
    "models                             print all the names of the models"));

    _commands.push_back(Command("material", [&](const std::string& _line){ 
        if (_line == "materials") {
            for (std::map<std::string,Material>::iterator it = m_materials.begin(); it != m_materials.end(); it++) {
                std::cout << it->second.name << std::endl;
            }
            return true;
        }
        else {
            std::vector<std::string> values = split(_line,',');
            if (values.size() == 2 && values[0] == "material") {
                for (std::map<std::string,Material>::iterator it = m_materials.begin(); it != m_materials.end(); it++){
                    if (it->second.name == values[1]) {
                        it->second.printDefines();
                        return true;
                    }
                }
            }
        }

        return false;
    },
    "models                             print all the names of the models"));

    _commands.push_back(Command("blend", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 1) {
            if (getBlend() == ALPHA) std::cout << "alpha" << std::endl;
            else if (getBlend() == ADD) std::cout << "add" << std::endl;
            else if (getBlend() == MULTIPLY) std::cout << "multiply" << std::endl;
            else if (getBlend() == SCREEN) std::cout << "screen" << std::endl;
            else if (getBlend() == SUBSTRACT) std::cout << "substract" << std::endl;
            
            return true;
        }
        else if (values.size() == 2) {
            if (values[1] == "alpha") setBlend(ALPHA);
            else if (values[1] == "add") setBlend(ADD);
            else if (values[1] == "multiply") setBlend(MULTIPLY);
            else if (values[1] == "screen") setBlend(SCREEN);
            else if (values[1] == "substract") setBlend(SUBSTRACT);

            return true;
        }

        return false;
    },
    "blend[,<alpha|add|multiply|screen|substract>]   get or set the blendign modes"));

    _commands.push_back(Command("depth_test", [&](const std::string& _line){ 
        if (_line == "depth_test") {
            std::string rta = m_depth_test ? "on" : "off";
            std::cout <<  rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = split(_line,',');
            if (values.size() == 2) {
                m_depth_test = (values[1] == "on");
                return true;
            }
        }
        return false;
    },
    "depth_test[,<on|off]   get or set the blendign modes"));

    _commands.push_back(Command("culling", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 1) {
            if (getCulling() == NONE) std::cout << "none" << std::endl;
            else if (getCulling() == FRONT) std::cout << "front" << std::endl;
            else if (getCulling() == BACK) std::cout << "back" << std::endl;
            else if (getCulling() == BOTH) std::cout << "both" << std::endl;

            return true;
        }
        else if (values.size() == 2) {
            if (values[1] == "none") setCulling(NONE);
            else if (values[1] == "front") setCulling(FRONT);
            else if (values[1] == "back") setCulling(BACK);
            else if (values[1] == "both") setCulling(BOTH);

            return true;
        }

        return false;
    },
    "culling[,<none|front|back|both>]   get or set the culling modes"));
    
    _commands.push_back(Command("dynamic_shadows", [&](const std::string& _line){ 
        if (_line == "dynamic_shadows") {
            std::string rta = m_dynamicShadows ? "on" : "off";
            std::cout <<  rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = split(_line,',');
            if (values.size() == 2) {
                m_dynamicShadows = (values[1] == "on");
                return true;
            }
        }
        return false;
    },
    "dynamic_shadows[on|off]            get or set dynamic shadows"));

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
    "skybox_ground[,<r>,<g>,<b>]        get or set the ground color of the skybox."));

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
    "skybox_azimuth[,<sun_azimuth>]     get or set the sun azimuth (in rads) of the skybox."));

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
            std::string rta = showCubebox ? "on" : "off";
            std::cout << rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = split(_line,',');
            if (values.size() == 2) {
                if (values[1] == "on")
                    setCubeMap(&m_skybox);
                showCubebox = values[1] == "on";
                return true;
            }
        }
        return false;
    },
    "skybox[,on|off]                    show/hide skybox"));

    _commands.push_back(Command("cubemap", [&](const std::string& _line){
        if (_line == "cubemap") {
            std::string rta = showCubebox ? "on" : "off";
            std::cout << rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = split(_line,',');
            if (values.size() == 2) {
                showCubebox = values[1] == "on";
                return true;
            }
        }
        return false;
    },
    "cubemap[,on|off]                   show/hide cubemap"));
    
    _commands.push_back(Command("model_position", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 4) {
            m_origin.setPosition( glm::vec3(toFloat(values[1]),toFloat(values[2]),toFloat(values[3])) );
            return true;
        }
        else {
            glm::vec3 pos = m_origin.getPosition();
            std::cout << pos.x << ',' << pos.y << ',' << pos.z << std::endl;
            return true;
        }
        return false;
    },
    "model_position[,<x>,<y>,<z>]       get or set the model position."));

    _commands.push_back(Command("floor", [&](const std::string& _line) {
        std::vector<std::string> values = split(_line,',');

        if (_line == "floor") {
            std::string rta = m_floor_subd > 0 ? toString(m_floor_subd) : "off";
            std::cout << rta << std::endl; 
            return true;
        }
        else {
            if (values.size() == 2) {
                if (values[1] == "off") {
                    m_floor_subd_target = -1;
                }
                else if (values[1] == "on") {
                    if (m_floor_subd_target == -1)
                        m_floor_subd_target = 0;
                }
                else {
                    m_floor_subd_target = toInt(values[1]);
                }
                return true;
            }
        }
        return false;
    },
    "floor[,on|off|subD_level]          show/hide floor or presice the subdivision level"));

    _commands.push_back(Command("grid", [&](const std::string& _line){
        if (_line == "grid") {
            std::string rta = showGrid ? "on" : "off";
            std::cout << rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = split(_line,',');
            if (values.size() == 2) {
                showGrid = values[1] == "on";
                return true;
            }
        }
        return false;
    },
    "grid[,on|off]                      show/hide grid"));

    _commands.push_back(Command("axis", [&](const std::string& _line){
        if (_line == "grid") {
            std::string rta = showAxis ? "on" : "off";
            std::cout << rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = split(_line,',');
            if (values.size() == 2) {
                showAxis = values[1] == "on";
                return true;
            }
        }
        return false;
    },
    "axis[,on|off]                      show/hide axis"));

    _commands.push_back(Command("bboxes", [&](const std::string& _line){
        if (_line == "bboxes") {
            std::string rta = showBBoxes ? "on" : "off";
            std::cout << rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = split(_line,',');
            if (values.size() == 2) {
                showBBoxes = values[1] == "on";
                return true;
            }
        }
        return false;
    },
    "bboxes[,on|off]                    show/hide models bounding boxes"));
    
    _uniforms.functions["u_model"] = UniformFunction("vec3", [this](Shader& _shader) {
        _shader.setUniform("u_model", m_origin.getPosition());
    },
    [this]() { return toString(m_origin.getPosition(), ','); });

    _uniforms.functions["u_modelMatrix"] = UniformFunction("mat4", [this](Shader& _shader) {
        _shader.setUniform("u_modelMatrix", m_origin.getTransformMatrix() );
    });
    
}

void Scene::addDefine(const std::string& _define, const std::string& _value) {
    m_background_shader.addDefine(_define, _value);
    m_floor_shader.addDefine(_define, _value);
    for (unsigned int i = 0; i < m_models.size(); i++) {
        m_models[i]->addDefine(_define, _value);
    }
}

void Scene::delDefine(const std::string& _define) {
    m_background_shader.delDefine(_define);
    m_floor_shader.delDefine(_define);
    for (unsigned int i = 0; i < m_models.size(); i++) {
        m_models[i]->delDefine(_define);
    }
}

void Scene::setCubeMap( SkyBox* _skybox ) { 
    if (m_cubemap_skybox)
        delete m_cubemap_skybox;

    m_cubemap_skybox = _skybox; 
    m_cubemap_skybox->change = true;
    flagChange();
}

void Scene::printDefines() {
    if (m_background) {
        std::cout << std::endl;
        std::cout << "BACKGROUND" << std::endl;
        std::cout << "-------------- " << std::endl;
        m_background_shader.printDefines();
    }

    if (m_floor_subd >= 0) {
        std::cout << std::endl;
        std::cout << "FLOOR" << std::endl;
        std::cout << "-------------- " << std::endl;
        m_floor_shader.printDefines();
    }

    for (unsigned int i = 0; i < m_models.size(); i++) {
        std::cout << std::endl;
        std::cout << m_models[i]->getName() << std::endl;
        std::cout << "-------------- " << std::endl;
        m_models[i]->printDefines();
    }
}

bool Scene::loadGeometry(Uniforms& _uniforms, WatchFileList& _files, int _index, bool _verbose) {
    std::string ext = getExt(_files[_index].path);

    // If the geometry is a PLY it's easy because is only one mesh
    if ( ext == "ply" || ext == "PLY" ) {
        loadPLY(_uniforms, _files, m_materials, m_models, _index, _verbose);

    }

    // If it's a OBJ could be more complicated because they can contain several meshes and materials
    else if ( ext == "obj" || ext == "OBJ" )
        loadOBJ(_uniforms, _files, m_materials, m_models, _index, _verbose);

    // If it's a GLTF it's not just multiple meshes and materials but also nodes, lights and cameras
    else if ( ext == "glb" || ext == "GLB" || ext == "gltf" || ext == "GLTF" )
        loadGLTF(_uniforms, _files, m_materials, m_models, _index, _verbose);

    // Calculate the total area
    glm::vec3 min_v;
    glm::vec3 max_v;
    for (unsigned int i = 0; i < m_models.size(); i++) {
        expandBoundingBox( m_models[i]->getMinBoundingBox(), min_v, max_v);
        expandBoundingBox( m_models[i]->getMaxBoundingBox(), min_v, max_v);
    }
    m_area = glm::max(0.5f, glm::max(glm::length(min_v), glm::length(max_v)));
    glm::vec3 centroid = (min_v + max_v) * 0.5f;
    m_origin.setPosition( -centroid );
    m_floor_height = min_v.y;

    _uniforms.getCamera().setPosition(glm::vec3(0.0,0.0,-m_area * 2.0));

    // Setup light
    if (_uniforms.lights.size() == 0)
        _uniforms.lights.push_back( Light( glm::vec3(0.0,m_area*100.0,m_area*100.0), -1.0 ) );

    return true;
}

bool Scene::loadShaders(const std::string& _fragmentShader, const std::string& _vertexShader, bool _verbose) {
    bool rta = true;
    for (unsigned int i = 0; i < m_models.size(); i++)
        if ( !m_models[i]->loadShader( _fragmentShader, _vertexShader, _verbose) )
            rta = false;


    m_background = check_for_background(_fragmentShader);
    if (m_background) {
        // Specific defines for this buffer
        m_background_shader.addDefine("BACKGROUND");
        m_background_shader.load(_fragmentShader, billboard_vert, false);
    }

    bool thereIsFloorDefine = check_for_floor(_fragmentShader) || check_for_floor(_vertexShader);
    if (thereIsFloorDefine) {
        m_floor_shader.load(_fragmentShader, _vertexShader, false);
        if (m_floor_subd == -1)
            m_floor_subd_target = 0;
    }

    return rta;
}

void Scene::flagChange() {
    m_origin.bChange = true;
}

bool Scene::haveChange() const {
    return  m_origin.bChange;
}

void Scene::unflagChange() { 
    m_origin.bChange = false;
}

void Scene::render(Uniforms& _uniforms) {
    // Render Background
    renderBackground(_uniforms);

    // Begining of DEPTH for 3D 
    if (m_depth_test)
        glEnable(GL_DEPTH_TEST);

    if (m_blend != 0) {
        switch (m_blend) {
            case 1: // Add
                glEnable(GL_BLEND);
                glBlendEquation(GL_FUNC_ADD);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                break;

            case 2: // Multiply
                glEnable(GL_BLEND);
                glBlendEquation(GL_FUNC_ADD);
                glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA /* GL_ZERO or GL_ONE_MINUS_SRC_ALPHA */);
                break;

            case 3: // Screen
                glEnable(GL_BLEND);
                glBlendEquation(GL_FUNC_ADD);
                glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE);
                break;

            case 4: // Substract
                glEnable(GL_BLEND);
                glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                break;
            default:
			    break;
	    }
    }

    if (_uniforms.getCamera().bChange || m_origin.bChange)
        m_mvp = _uniforms.getCamera().getProjectionViewMatrix() * m_origin.getTransformMatrix(); 
    
    renderFloor(_uniforms, m_mvp);

    if (m_culling != 0) {
        glEnable(GL_CULL_FACE);

        if (m_culling == 1) 
            glCullFace(GL_FRONT);
        
        else if (m_culling == 2)
            glCullFace(GL_BACK);
        
        else if (m_culling == 3)
            glCullFace(GL_FRONT_AND_BACK);

    }

    for (unsigned int i = 0; i < m_models.size(); i++)
        m_models[i]->render(_uniforms, m_mvp);

    if (m_depth_test)
        glDisable(GL_DEPTH_TEST);

    if (m_blend != 0) {
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    if (m_culling != 0)
        glDisable(GL_CULL_FACE);
}

void Scene::renderShadowMap(Uniforms& _uniforms) {
#if defined(PLATFORM_RPI) || defined(PLATFORM_RPI4) 
    return;
#else

    bool changeOnLights = false;
    for (unsigned int i = 0; i < _uniforms.lights.size(); i++) {
        if (_uniforms.lights[i].bChange) {
            changeOnLights = true;
            break;
        }
    }

    if ( m_dynamicShadows || changeOnLights || m_origin.bChange ) {
        for (unsigned int i = 0; i < _uniforms.lights.size(); i++) {
            // Temporally move the MVP matrix from the view of the light 
            glm::mat4 mvp = _uniforms.lights[i].getMVPMatrix( m_origin.getTransformMatrix(), m_area );
            _uniforms.lights[i].bindShadowMap();

            renderFloor(_uniforms, mvp);

            for (unsigned int i = 0; i < m_models.size(); i++)
                m_models[i]->render(_uniforms, mvp);

            _uniforms.lights[i].unbindShadowMap();
        }
    }
#endif
}

void Scene::renderBackground(Uniforms& _uniforms) {
    // If there is a skybox and it had changes re generate
    if (m_cubemap_skybox) {
        if (m_cubemap_skybox->change) {
            if (!_uniforms.cubemap) {
                _uniforms.cubemap = new TextureCube();
            }
            _uniforms.cubemap->generate(m_cubemap_skybox);
            m_cubemap_skybox->change = false;
        }
    }

    if (m_background) {
        m_background_shader.use();

        // Update Uniforms and textures
        _uniforms.feedTo( m_background_shader );

        if (!m_background_vbo)
            m_background_vbo = rect(0.0,0.0,1.0,1.0).getVbo();
        m_background_vbo->render( &m_background_shader );
    }

    else if (_uniforms.cubemap && showCubebox) {
        if (!m_cubemap_vbo) {
            m_cubemap_vbo = cube(1.0f).getVbo();
            m_cubemap_shader.load(cube_frag, cube_vert, false);
        }

        m_cubemap_shader.use();

        m_cubemap_shader.setUniform("u_modelViewProjectionMatrix", _uniforms.getCamera().getProjectionMatrix() * glm::toMat4(_uniforms.getCamera().getOrientationQuat()) );
        m_cubemap_shader.setUniformTextureCube( "u_cubeMap", _uniforms.cubemap, 0 );

        m_cubemap_vbo->render( &m_cubemap_shader );
    }
}

void Scene::renderFloor(Uniforms& _uniforms, const glm::mat4& _mvp) {
    if (m_floor_subd_target >= 0) {

        //  Floor
        if (m_floor_subd_target != m_floor_subd) {

            if (m_floor_vbo)
                delete m_floor_vbo;

            m_floor_vbo = floor(m_area * 5.0, m_floor_subd_target, m_floor_height).getVbo();
            m_floor_subd = m_floor_subd_target;

            if (!m_floor_shader.isLoaded()) 
                m_floor_shader.load(default_scene_frag, default_scene_vert, false);

            m_floor_shader.addDefine("FLOOR");
            m_floor_shader.addDefine("FLOOR_SUBD", m_floor_subd);
            m_floor_shader.addDefine("FLOOR_AREA", m_area * 3.0f);
            m_floor_shader.addDefine("FLOOR_HEIGHT", m_floor_height);

            m_floor_shader.addDefine("MODEL_VERTEX_COLOR");
            m_floor_shader.addDefine("MODEL_VERTEX_NORMAL");
            m_floor_shader.addDefine("MODEL_VERTEX_TEXCOORD");
            
            m_floor_shader.addDefine("LIGHT_SHADOWMAP", "u_lightShadowMap");
            #if defined(PLATFORM_RPI) || defined(PLATFORM_RPI4) 
                m_floor_shader.addDefine("LIGHT_SHADOWMAP_SIZE", "512.0");
            #else
                m_floor_shader.addDefine("LIGHT_SHADOWMAP_SIZE", "1024.0");
            #endif
        }

        if (m_floor_vbo) {
            m_floor_shader.use();
            _uniforms.feedTo( m_floor_shader );
            m_floor_shader.setUniform("u_modelViewProjectionMatrix", _mvp );
            m_floor_vbo->render( &m_floor_shader );
        }
    }
}


void Scene::renderDebug(Uniforms& _uniforms) {
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    if (!m_wireframe3D_shader.isLoaded())
        m_wireframe3D_shader.load(wireframe3D_frag, wireframe3D_vert, false);

    // Draw Bounding boxes
    if (showBBoxes) {
        glLineWidth(3.0f);
        m_wireframe3D_shader.use();
        m_wireframe3D_shader.setUniform("u_color", glm::vec4(1.0,1.0,0.0,1.0));
        m_wireframe3D_shader.setUniform("u_modelViewProjectionMatrix", m_mvp );
        for (unsigned int i = 0; i < m_models.size(); i++) {
            m_models[i]->renderBbox( &m_wireframe3D_shader );
        }
    }
    
    // Axis
    if (showAxis) {
        if (m_axis_vbo == nullptr)
            m_axis_vbo = axis(_uniforms.getCamera().getFarClip(), m_floor_height).getVbo();

        glLineWidth(2.0f);
        m_wireframe3D_shader.use();
        m_wireframe3D_shader.setUniform("u_color", glm::vec4(0.75));
        m_wireframe3D_shader.setUniform("u_modelViewProjectionMatrix", m_mvp );
        m_axis_vbo->render( &m_wireframe3D_shader );
    }
    
    // Grid
    if (showGrid) {
        if (m_grid_vbo == nullptr)
            m_grid_vbo = grid(_uniforms.getCamera().getFarClip(), _uniforms.getCamera().getFarClip() / 20.0, m_floor_height).getVbo();
        glLineWidth(1.0f);
        m_wireframe3D_shader.use();
        m_wireframe3D_shader.setUniform("u_color", glm::vec4(0.5));
        m_wireframe3D_shader.setUniform("u_modelViewProjectionMatrix", m_mvp );
        m_grid_vbo->render( &m_wireframe3D_shader );
    }


    // Light
    {
        if (!m_lightUI_shader.isLoaded())
            m_lightUI_shader.load(light_frag, light_vert, false);

        if (m_lightUI_vbo == nullptr)
            m_lightUI_vbo = rect(0.0,0.0,0.0,0.0).getVbo();

        m_lightUI_shader.use();
        m_lightUI_shader.setUniform("u_scale", 24.0f, 24.0f);
        m_lightUI_shader.setUniform("u_viewMatrix", _uniforms.getCamera().getViewMatrix());
        m_lightUI_shader.setUniform("u_modelViewProjectionMatrix", m_mvp );

        for (unsigned int i = 0; i < _uniforms.lights.size(); i++) {
            m_lightUI_shader.setUniform("u_translate", _uniforms.lights[i].getPosition());
            m_lightUI_shader.setUniform("u_color", glm::vec4(_uniforms.lights[i].color, 1.0));

            m_lightUI_vbo->render( &m_lightUI_shader );
        }
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
}
