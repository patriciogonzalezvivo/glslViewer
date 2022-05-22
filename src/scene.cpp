
#include "scene.h"

#ifndef PLATFORM_WINDOWS
#include <unistd.h>
#endif

#include <sys/stat.h>

#include "ada/window.h"
#include "ada/gl/draw.h"
#include "ada/gl/meshes.h"
#include "ada/tools/fs.h"
#include "ada/tools/geom.h"
#include "ada/tools/text.h"
#include "ada/shaders/defaultShaders.h"

#include "io/ply.h"
#include "io/obj.h"
#include "io/gltf.h"
#include "io/stl.h"

#include "tools/text.h"

#define TRACK_BEGIN(A) if (_uniforms.tracker.isRunning()) _uniforms.tracker.begin(A); 
#define TRACK_END(A) if (_uniforms.tracker.isRunning()) _uniforms.tracker.end(A); 

Scene::Scene(): 
    // Debug State
    showGrid(false), showAxis(false), showBBoxes(false), showCubebox(false), 
    // Camera.
    m_blend(ada::BLEND_ALPHA), m_culling(ada::CULL_NONE), m_depth_test(true),
    // Light
    m_lightUI_vbo(nullptr), m_dynamicShadows(false), m_shadows(false),
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
            std::vector<std::string> values = ada::split(_line,',');
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
    "models", "print all the names of the models"));

    _commands.push_back(Command("material", [&](const std::string& _line){ 
        if (_line == "materials") {
            for (std::map<std::string,ada::Material>::iterator it = m_materials.begin(); it != m_materials.end(); it++) {
                std::cout << it->second.name << std::endl;
            }
            return true;
        }
        else {
            std::vector<std::string> values = ada::split(_line,',');
            if (values.size() == 2 && values[0] == "material") {
                for (std::map<std::string,ada::Material>::iterator it = m_materials.begin(); it != m_materials.end(); it++){
                    if (it->second.name == values[1]) {
                        it->second.printDefines();
                        return true;
                    }
                }
            }
        }

        return false;
    },
    "materials", "print all the materials names"));

    _commands.push_back(Command("blend", [&](const std::string& _line){ 
        std::vector<std::string> values = ada::split(_line,',');
        if (values.size() == 1) {
            if (getBlend() == ada::BLEND_NONE) std::cout << "none" << std::endl;
            else if (getBlend() == ada::BLEND_ALPHA) std::cout << "alpha" << std::endl;
            else if (getBlend() == ada::BLEND_ADD) std::cout << "add" << std::endl;
            else if (getBlend() == ada::BLEND_MULTIPLY) std::cout << "multiply" << std::endl;
            else if (getBlend() == ada::BLEND_SCREEN) std::cout << "screen" << std::endl;
            else if (getBlend() == ada::BLEND_SUBSTRACT) std::cout << "substract" << std::endl;
            
            return true;
        }
        else if (values.size() == 2) {
            if (values[1] == "none" || values[1] == "off") setBlend(ada::BLEND_NONE);
            else if (values[1] == "alpha") setBlend(ada::BLEND_ALPHA);
            else if (values[1] == "add") setBlend(ada::BLEND_ADD);
            else if (values[1] == "multiply") setBlend(ada::BLEND_MULTIPLY);
            else if (values[1] == "screen") setBlend(ada::BLEND_SCREEN);
            else if (values[1] == "substract") setBlend(ada::BLEND_SUBSTRACT);

            return true;
        }

        return false;
    },
    "blend[,alpha|add|multiply|screen|substract]", "get or set the blending modes"));

    _commands.push_back(Command("depth_test", [&](const std::string& _line){ 
        if (_line == "depth_test") {
            std::string rta = m_depth_test ? "on" : "off";
            std::cout <<  rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = ada::split(_line,',');
            if (values.size() == 2) {
                m_depth_test = (values[1] == "on");
                return true;
            }
        }
        return false;
    },
    "depth_test[,on|off]", "turn on/off depth test"));

    _commands.push_back(Command("culling", [&](const std::string& _line){ 
        std::vector<std::string> values = ada::split(_line,',');
        if (values.size() == 1) {
            if (getCulling() == ada::CULL_NONE) std::cout << "none" << std::endl;
            else if (getCulling() == ada::CULL_FRONT) std::cout << "front" << std::endl;
            else if (getCulling() == ada::CULL_BACK) std::cout << "back" << std::endl;
            else if (getCulling() == ada::CULL_BOTH) std::cout << "both" << std::endl;

            return true;
        }
        else if (values.size() == 2) {
            if (values[1] == "none") setCulling(ada::CULL_NONE);
            else if (values[1] == "front") setCulling(ada::CULL_FRONT);
            else if (values[1] == "back") setCulling(ada::CULL_BACK);
            else if (values[1] == "both") setCulling(ada::CULL_BOTH);

            return true;
        }

        return false;
    },
    "culling[,none|front|back|both]", "get or set the culling modes"));
    
    _commands.push_back(Command("dynamic_shadows", [&](const std::string& _line){ 
        if (_line == "dynamic_shadows") {
            std::string rta = m_dynamicShadows ? "on" : "off";
            std::cout <<  rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = ada::split(_line,',');
            if (values.size() == 2) {
                m_dynamicShadows = (values[1] == "on");
                return true;
            }
        }
        return false;
    },
    "dynamic_shadows[,on|off]", "get or set dynamic shadows"));

    _commands.push_back(Command("skybox_ground", [&](const std::string& _line){ 
        std::vector<std::string> values = ada::split(_line,',');
        if (values.size() == 4) {
            m_skybox.groundAlbedo = glm::vec3(ada::toFloat(values[1]), ada::toFloat(values[2]), ada::toFloat(values[3]));
            setCubeMap(&m_skybox);
            return true;
        }
        else {
            std::cout << m_skybox.groundAlbedo.x << ',' << m_skybox.groundAlbedo.y << ',' << m_skybox.groundAlbedo.z << std::endl;
            return true;
        }
        return false;
    },
    "skybox_ground[,<r>,<g>,<b>]", "get or set the ground color of the skybox."));

    _commands.push_back(Command("skybox_elevation", [&](const std::string& _line){ 
        std::vector<std::string> values = ada::split(_line,',');
        if (values.size() == 2) {
            m_skybox.elevation = ada::toFloat(values[1]);
            setCubeMap(&m_skybox);
            return true;
        }
        else {
            std::cout << m_skybox.elevation << std::endl;
            return true;
        }
        return false;
    },
    "skybox_elevation[,<sun_elevation>]", "get or set the sun elevation (in rads) of the skybox."));

    _commands.push_back(Command("skybox_azimuth", [&](const std::string& _line){ 
        std::vector<std::string> values = ada::split(_line,',');
        if (values.size() == 2) {
            m_skybox.azimuth = ada::toFloat(values[1]);
            setCubeMap(&m_skybox);
            return true;
        }
        else {
            std::cout << m_skybox.azimuth << std::endl;
            return true;
        }
        return false;
    },
    "skybox_azimuth[,<sun_azimuth>]", "get or set the sun azimuth (in rads) of the skybox."));

    _commands.push_back(Command("skybox_turbidity", [&](const std::string& _line){ 
        std::vector<std::string> values = ada::split(_line,',');
        if (values.size() == 2) {
            m_skybox.turbidity = ada::toFloat(values[1]);
            setCubeMap(&m_skybox);
            return true;
        }
        else {
            std::cout << m_skybox.turbidity << std::endl;
            return true;
        }
        return false;
    },
    "skybox_turbidity[,<sky_turbidty>]", "get or set the sky turbidity of the m_skybox."));

    _commands.push_back(Command("skybox", [&](const std::string& _line){
        if (_line == "skybox") {
            std::string rta = showCubebox ? "on" : "off";
            std::cout << rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = ada::split(_line,',');
            if (values.size() == 2) {
                if (values[1] == "on")
                    setCubeMap(&m_skybox);
                showCubebox = values[1] == "on";
                return true;
            }
        }
        return false;
    },
    "skybox[,on|off]", "show/hide skybox"));

    _commands.push_back(Command("cubemap", [&](const std::string& _line){
        if (_line == "cubemap") {
            std::string rta = showCubebox ? "on" : "off";
            std::cout << rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = ada::split(_line,',');
            if (values.size() == 2) {
                showCubebox = values[1] == "on";
                return true;
            }
        }
        return false;
    },
    "cubemap[,on|off]", "show/hide cubemap"));
    
    _commands.push_back(Command("model_position", [&](const std::string& _line){ 
        std::vector<std::string> values = ada::split(_line,',');
        if (values.size() == 4) {
            m_origin.setPosition( glm::vec3(ada::toFloat(values[1]), ada::toFloat(values[2]), ada::toFloat(values[3])) );
            return true;
        }
        else {
            glm::vec3 pos = m_origin.getPosition();
            std::cout << pos.x << ',' << pos.y << ',' << pos.z << std::endl;
            return true;
        }
        return false;
    },
    "model_position[,<x>,<y>,<z>]", "get or set the model position."));

    _commands.push_back(Command("floor", [&](const std::string& _line) {
        std::vector<std::string> values = ada::split(_line,',');

        if (_line == "floor") {
            std::string rta = m_floor_subd > 0 ? ada::toString(m_floor_subd) : "off";
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
                    m_floor_subd_target = ada::toInt(values[1]);
                }
                return true;
            }
        }
        return false;
    },
    "floor[,on|off|<subD_level>]", "show/hide floor or presice the subdivision level"));

    _commands.push_back(Command("grid", [&](const std::string& _line){
        if (_line == "grid") {
            std::string rta = showGrid ? "on" : "off";
            std::cout << rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = ada::split(_line,',');
            if (values.size() == 2) {
                showGrid = values[1] == "on";
                return true;
            }
        }
        return false;
    },
    "grid[,on|off]", "show/hide grid"));

    _commands.push_back(Command("axis", [&](const std::string& _line){
        if (_line == "grid") {
            std::string rta = showAxis ? "on" : "off";
            std::cout << rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = ada::split(_line,',');
            if (values.size() == 2) {
                showAxis = values[1] == "on";
                return true;
            }
        }
        return false;
    },
    "axis[,on|off]", "show/hide axis"));

    _commands.push_back(Command("bboxes", [&](const std::string& _line){
        if (_line == "bboxes") {
            std::string rta = showBBoxes ? "on" : "off";
            std::cout << rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = ada::split(_line,',');
            if (values.size() == 2) {
                showBBoxes = values[1] == "on";
                return true;
            }
        }
        return false;
    },
    "bboxes[,on|off]", "show/hide models bounding boxes"));
    
    _uniforms.functions["u_model"] = UniformFunction("vec3", [this](ada::Shader& _shader) {
        _shader.setUniform("u_model", m_origin.getPosition());
    },
    [this]() { return ada::toString(m_origin.getPosition(), ','); });

    _uniforms.functions["u_modelMatrix"] = UniformFunction("mat4", [this](ada::Shader& _shader) {
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

void Scene::setCubeMap(ada::SkyData* _skybox ) { 
    if (m_cubemap_skybox)
        if (m_cubemap_skybox != _skybox)
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
    std::string ext = ada::getExt(_files[_index].path);

    // If the geometry is a PLY it's easy because is only one mesh
    if ( ext == "ply" || ext == "PLY" )
        loadPLY(_uniforms, _files, m_materials, m_models, _index, _verbose);

    // If it's a OBJ could be more complicated because they can contain several meshes and materials
    else if ( ext == "obj" || ext == "OBJ" )
        loadOBJ(_uniforms, _files, m_materials, m_models, _index, _verbose);

    // If it's a GLTF it's not just multiple meshes and materials but also nodes, lights and cameras
    else if ( ext == "glb" || ext == "GLB" || ext == "gltf" || ext == "GLTF" )
        loadGLTF(_uniforms, _files, m_materials, m_models, _index, _verbose);

    // If it's a STL 
    else if ( ext == "stl" || ext == "STL" )
        loadSTL(_files, m_materials, m_models, _index, _verbose);

    // Calculate the total area
    glm::vec3 min_v;
    glm::vec3 max_v;
    for (unsigned int i = 0; i < m_models.size(); i++) {
        ada::expandBoundingBox( m_models[i]->getMinBoundingBox(), min_v, max_v);
        ada::expandBoundingBox( m_models[i]->getMaxBoundingBox(), min_v, max_v);
    }
    m_area = glm::max(0.5f, glm::max(glm::length(min_v), glm::length(max_v)));
    glm::vec3 centroid = (min_v + max_v) * 0.5f;
    m_origin.setPosition( -centroid );
    m_floor_height = min_v.y;

    // Setup light
    if (_uniforms.lights.size() == 0)
        _uniforms.lights.push_back( ada::Light( glm::vec3(0.0,m_area*100.0,m_area*100.0), -1.0 ) );

    return true;
}

bool Scene::loadShaders(const std::string& _fragmentShader, const std::string& _vertexShader, bool _verbose) {
    bool rta = true;
    for (unsigned int i = 0; i < m_models.size(); i++)
        if ( !m_models[i]->loadShader( _fragmentShader, _vertexShader, _verbose) )
            rta = false;


    m_background = checkBackground(_fragmentShader);
    if (m_background) {
        // Specific defines for this buffer
        m_background_shader.addDefine("BACKGROUND");
        m_background_shader.load(_fragmentShader, ada::getDefaultSrc(ada::VERT_BILLBOARD), false);
    }

    bool thereIsFloorDefine = checkFloor(_fragmentShader) || checkFloor(_vertexShader);
    if (thereIsFloorDefine) {
        m_floor_shader.load(_fragmentShader, _vertexShader, false);
        if (m_floor_subd == -1)
            m_floor_subd_target = 0;
    }

    m_shadows = findId(_fragmentShader, "u_lightShadowMap;");

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

    ada::blendMode(m_blend);

    if (_uniforms.getCamera().bChange || m_origin.bChange)
        ada::setCameraMatrix( _uniforms.getCamera().getProjectionViewMatrix() * m_origin.getTransformMatrix() );
    ada::resetMatrix();

    TRACK_BEGIN("render:scene:floor")
    renderFloor(_uniforms, ada::getCameraMatrix() );
    TRACK_END("render:scene:floor")

    ada::cullingMode(m_culling);

    for (size_t i = 0; i < m_models.size(); i++) {

        if (m_models[i]->getShader()->isLoaded() ) {

            TRACK_BEGIN("render:scene:" + m_models[i]->getName() )

            // bind the shader
            m_models[i]->getShader()->use();

            // Update Uniforms and textures variables to the shader
            _uniforms.feedTo( m_models[i]->getShader() );

            // Pass special uniforms
            m_models[i]->getShader()->setUniform( "u_modelViewProjectionMatrix", ada::getCameraMatrix() );
            m_models[i]->render();

            TRACK_END("render:scene:" + m_models[i]->getName() )
        }
    }

    if (m_depth_test)
        glDisable(GL_DEPTH_TEST);

    if (m_blend != 0) {
        // glEnable(GL_BLEND);
        // glBlendEquation(GL_FUNC_ADD);
        // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        ada::blendMode(ada::BLEND_ALPHA);
    }

    if (m_culling != 0)
        glDisable(GL_CULL_FACE);
}

void Scene::renderShadowMap(Uniforms& _uniforms) {
    if (!m_shadows)
        return;

    bool changeOnLights = false;
    for (unsigned int i = 0; i < _uniforms.lights.size(); i++) {
        if (_uniforms.lights[i].bChange) {
            changeOnLights = true;
            break;
        }
    }

    if ( m_dynamicShadows || changeOnLights || m_origin.bChange ) {
        // TRACK_BEGIN("shadowmap")
        for (unsigned int i = 0; i < _uniforms.lights.size(); i++) {
            // Temporally move the MVP matrix from the view of the light 
            glm::mat4 mvp = _uniforms.lights[i].getMVPMatrix( m_origin.getTransformMatrix(), m_area );
            _uniforms.lights[i].bindShadowMap();

            renderFloor(_uniforms, mvp);

            for (unsigned int i = 0; i < m_models.size(); i++) {
                // m_models[i]->render(_uniforms, mvp);
                if (m_models[i]->getShader()->isLoaded() ) {

                    // bind the shader
                    m_models[i]->getShader()->use();

                    // Update Uniforms and textures variables to the shader
                    _uniforms.feedTo( m_models[i]->getShader(), false );

                    // Pass special uniforms
                    m_models[i]->getShader()->setUniform( "u_modelViewProjectionMatrix", mvp);
                    m_models[i]->render();
                }
            }

            _uniforms.lights[i].unbindShadowMap();
        }
        // TRACK_END("shadowmap")
    }
}

void Scene::renderBackground(Uniforms& _uniforms) {
    // If there is a skybox and it had changes re generate
    if (m_cubemap_skybox) {
        if (m_cubemap_skybox->change) {
            if (!_uniforms.cubemap) {
                _uniforms.cubemap = new ada::TextureCube();
            }
            _uniforms.cubemap->load(m_cubemap_skybox);
            m_cubemap_skybox->change = false;
        }
    }

    if (m_background) {
        TRACK_BEGIN("render:scene:background")

        m_background_shader.use();

        // Update Uniforms and textures
        _uniforms.feedTo( m_background_shader );

        if (!m_background_vbo)
            m_background_vbo = ada::rectMesh(0.0,0.0,1.0,1.0).getVbo();
        m_background_vbo->render( &m_background_shader );

        TRACK_END("render:scene:background")
    }

    else if (_uniforms.cubemap && showCubebox) {

        if (!m_cubemap_vbo) {
            m_cubemap_vbo = ada::cubeMesh(1.0f).getVbo();
            m_cubemap_shader.load(ada::getDefaultSrc(ada::FRAG_CUBEMAP), ada::getDefaultSrc(ada::VERT_CUBEMAP), false);
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

            m_floor_vbo = ada::floorMesh(m_area * 5.0, m_floor_subd_target, m_floor_height).getVbo();
            m_floor_subd = m_floor_subd_target;

            if (!m_floor_shader.isLoaded()) 
                m_floor_shader.load(ada::getDefaultSrc(ada::FRAG_DEFAULT_SCENE), ada::getDefaultSrc(ada::VERT_DEFAULT_SCENE), false);

            m_floor_shader.addDefine("FLOOR");
            m_floor_shader.addDefine("FLOOR_SUBD", m_floor_subd);
            m_floor_shader.addDefine("FLOOR_AREA", m_area * 3.0f);
            m_floor_shader.addDefine("FLOOR_HEIGHT", m_floor_height);

            m_floor_shader.addDefine("MODEL_VERTEX_COLOR");
            m_floor_shader.addDefine("MODEL_VERTEX_NORMAL");
            m_floor_shader.addDefine("MODEL_VERTEX_TEXCOORD");
            
            m_floor_shader.addDefine("LIGHT_SHADOWMAP", "u_lightShadowMap");
            #if defined(PLATFORM_RPI)
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
    ada::camera(true);

    // Draw Bounding boxes
    if (showBBoxes) {
        ada::strokeWeight(3.0f);
        ada::stroke(glm::vec3(1.0f, 0.0f, 0.0f));
        for (unsigned int i = 0; i < m_models.size(); i++)
            ada::line( m_models[i]->getVboBbox() );
    }
    
    // Axis
    if (showAxis) {
        if (m_axis_vbo == nullptr)
            m_axis_vbo = ada::axisMesh(_uniforms.getCamera().getFarClip(), m_floor_height).getVbo();

        ada::strokeWeight(2.0f);
        ada::stroke( glm::vec4(0.75f) );
        ada::line( m_axis_vbo );
    }
    
    // Grid
    if (showGrid) {
        if (m_grid_vbo == nullptr)
            m_grid_vbo = ada::gridMesh(_uniforms.getCamera().getFarClip(), _uniforms.getCamera().getFarClip() / 20.0, m_floor_height).getVbo();

        ada::strokeWeight(1.0f);
        ada::stroke( glm::vec4(0.5f) );
        ada::line( m_grid_vbo );
    }


    // Light
    {
        if (!m_lightUI_shader.isLoaded())
            m_lightUI_shader.load(ada::getDefaultSrc(ada::FRAG_LIGHT), ada::getDefaultSrc(ada::VERT_LIGHT), false);

        if (m_lightUI_vbo == nullptr)
            m_lightUI_vbo = ada::rectMesh(0.0,0.0,0.0,0.0).getVbo();

        m_lightUI_shader.use();
        m_lightUI_shader.setUniform("u_scale", 24.0f, 24.0f);
        m_lightUI_shader.setUniform("u_viewMatrix", _uniforms.getCamera().getViewMatrix());
        m_lightUI_shader.setUniform("u_modelViewProjectionMatrix", ada::getCameraMatrix() );

        for (unsigned int i = 0; i < _uniforms.lights.size(); i++) {
            m_lightUI_shader.setUniform("u_translate", _uniforms.lights[i].getPosition());
            m_lightUI_shader.setUniform("u_color", glm::vec4(_uniforms.lights[i].color, 1.0));

            m_lightUI_vbo->render( &m_lightUI_shader );
        }
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
}
