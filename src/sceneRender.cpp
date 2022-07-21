
#include "sceneRender.h"

#ifndef PLATFORM_WINDOWS
#include <unistd.h>
#endif

#include <sys/stat.h>

#include "vera/ops/fs.h"
#include "vera/ops/draw.h"
#include "vera/ops/math.h"
#include "vera/window.h"
#include "vera/ops/string.h"
#include "vera/ops/geom.h"
#include "vera/ops/meshes.h"
#include "vera/shaders/defaultShaders.h"
#include "vera/xr/xr.h"

#include "tools/text.h"

#define TRACK_BEGIN(A) if (_uniforms.tracker.isRunning()) _uniforms.tracker.begin(A); 
#define TRACK_END(A) if (_uniforms.tracker.isRunning()) _uniforms.tracker.end(A); 

SceneRender::SceneRender(): 
    // Debug State
    showGrid(false), showAxis(false), showBBoxes(false), showCubebox(false), 
    // Camera.
    m_blend(vera::BLEND_ALPHA), m_culling(vera::CULL_NONE), m_depth_test(true),
    // Light
    m_lightUI_vbo(nullptr), m_dynamicShadows(false), m_shadows(false),
    // Background
    m_background(false), 
    // Floor
    m_floor_vbo(nullptr), m_floor_height(0.0), m_floor_subd_target(-1), m_floor_subd(-1), 
    // UI
    m_grid_vbo(nullptr), m_axis_vbo(nullptr) 
    {
}

SceneRender::~SceneRender(){
    clear();
}

void SceneRender::clear() {
    if (m_lightUI_vbo) {
        delete m_lightUI_vbo;
        m_lightUI_vbo = nullptr;
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

void SceneRender::setup(CommandList& _commands, Uniforms& _uniforms) {
    // ADD COMMANDS
    // ----------------------------------------- 
    _commands.push_back(Command("model", [&](const std::string& _line){ 
        if (_line == "models") {
            for (vera::ModelsMap::iterator it = _uniforms.models.begin(); it != _uniforms.models.end(); ++it)
                std::cout << it->second->getName() << std::endl;
            return true;
        }
        else {
            std::vector<std::string> values = vera::split(_line,',');
            if (values.size() == 2 && values[0] == "model")
                for (vera::ModelsMap::iterator it = _uniforms.models.begin(); it != _uniforms.models.end(); ++it)
                    if (it->second->getName() == values[1]) {
                        it->second->printVboInfo();
                        std::cout << "Defines:" << std::endl;
                        it->second->printDefines();
                        return true;
                    }
        }

        return false;
    },
    "models", "print all the names of the models"));

    _commands.push_back(Command("material", [&](const std::string& _line){ 
        if (_line == "materials") {
            for (vera::MaterialsMap::iterator it = _uniforms.materials.begin(); it != _uniforms.materials.end(); it++)
                std::cout << it->second.name << std::endl;
            return true;
        }
        else {
            std::vector<std::string> values = vera::split(_line,',');
            if (values.size() == 2 && values[0] == "material") {
                for (vera::MaterialsMap::iterator it = _uniforms.materials.begin(); it != _uniforms.materials.end(); it++) {
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
        std::vector<std::string> values = vera::split(_line,',');
        if (values.size() == 1) {
            if (getBlend() == vera::BLEND_NONE) std::cout << "none" << std::endl;
            else if (getBlend() == vera::BLEND_ALPHA) std::cout << "alpha" << std::endl;
            else if (getBlend() == vera::BLEND_ADD) std::cout << "add" << std::endl;
            else if (getBlend() == vera::BLEND_MULTIPLY) std::cout << "multiply" << std::endl;
            else if (getBlend() == vera::BLEND_SCREEN) std::cout << "screen" << std::endl;
            else if (getBlend() == vera::BLEND_SUBSTRACT) std::cout << "substract" << std::endl;
            
            return true;
        }
        else if (values.size() == 2) {
            if (values[1] == "none" || values[1] == "off") setBlend(vera::BLEND_NONE);
            else if (values[1] == "alpha") setBlend(vera::BLEND_ALPHA);
            else if (values[1] == "add") setBlend(vera::BLEND_ADD);
            else if (values[1] == "multiply") setBlend(vera::BLEND_MULTIPLY);
            else if (values[1] == "screen") setBlend(vera::BLEND_SCREEN);
            else if (values[1] == "substract") setBlend(vera::BLEND_SUBSTRACT);

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
            std::vector<std::string> values = vera::split(_line,',');
            if (values.size() == 2) {
                m_depth_test = (values[1] == "on");
                return true;
            }
        }
        return false;
    },
    "depth_test[,on|off]", "turn on/off depth test"));

    _commands.push_back(Command("culling", [&](const std::string& _line){ 
        std::vector<std::string> values = vera::split(_line,',');
        if (values.size() == 1) {
            if (getCulling() == vera::CULL_NONE) std::cout << "none" << std::endl;
            else if (getCulling() == vera::CULL_FRONT) std::cout << "front" << std::endl;
            else if (getCulling() == vera::CULL_BACK) std::cout << "back" << std::endl;
            else if (getCulling() == vera::CULL_BOTH) std::cout << "both" << std::endl;

            return true;
        }
        else if (values.size() == 2) {
            if (values[1] == "none") setCulling(vera::CULL_NONE);
            else if (values[1] == "front") setCulling(vera::CULL_FRONT);
            else if (values[1] == "back") setCulling(vera::CULL_BACK);
            else if (values[1] == "both") setCulling(vera::CULL_BOTH);

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
            std::vector<std::string> values = vera::split(_line,',');
            if (values.size() == 2) {
                m_dynamicShadows = (values[1] == "on");
                return true;
            }
        }
        return false;
    },
    "dynamic_shadows[,on|off]", "get or set dynamic shadows"));

    _commands.push_back(Command("floor_color", [&](const std::string& _line){ 
        std::vector<std::string> values = vera::split(_line,',');
        if (values.size() == 4) {
            std::string str_color = "vec4("+values[1]+","+values[2]+","+values[3]+",1.0)"; 
            addDefine("FLOOR_COLOR",str_color);
            
            _uniforms.setGroundAlbedo( glm::vec3(vera::toFloat(values[1]), vera::toFloat(values[2]), vera::toFloat(values[3])) );
            _uniforms.activeCubemap = _uniforms.cubemaps["default"];
            return true;
        }
        else {
            glm::vec3 ground = _uniforms.getGroundAlbedo();
            std::cout << ground.x << ',' << ground.y << ',' << ground.z << std::endl;
            return true;
        }
        return false;
    },
    "floor_color[,<r>,<g>,<b>]", "get or set the ground color of the skybox."));
    
    _commands.push_back(Command("model_position", [&](const std::string& _line){ 
        std::vector<std::string> values = vera::split(_line,',');
        if (values.size() == 4) {
            m_origin.setPosition( glm::vec3(vera::toFloat(values[1]), vera::toFloat(values[2]), vera::toFloat(values[3])) );
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

    _commands.push_back(Command("cubemap", [&](const std::string& _line){
        if (_line == "cubemaps") {
            _uniforms.printCubemaps();
            return true;
        } else if (_line == "cubemap") {
            std::string rta = showCubebox ? "on" : "off";
            std::cout << rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = vera::split(_line,',');
            if (values.size() == 2) {
                showCubebox = values[1] == "on";
                return true;
            }
        }
        return false;
    },
    "cubemap[,on|off]", "show/hide cubemap"));

    _commands.push_back(Command("sky", [&](const std::string& _line){
        if (_line == "sky") {
            std::string rta = showCubebox ? "on" : "off";
            std::cout << rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = vera::split(_line,',');
            if (values.size() == 2) {
                if (values[1] == "on")
                    _uniforms.activeCubemap = _uniforms.cubemaps["default"];

                showCubebox = values[1] == "on";
                return true;
            }
        }
        return false;
    },
    "sky[,on|off]", "show/hide skybox"));

    _commands.push_back(Command("floor", [&](const std::string& _line) {
        std::vector<std::string> values = vera::split(_line,',');

        if (_line == "floor") {
            std::string rta = m_floor_subd > 0 ? vera::toString(m_floor_subd) : "off";
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
                    m_floor_subd_target = vera::toInt(values[1]);
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
            std::vector<std::string> values = vera::split(_line,',');
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
            std::vector<std::string> values = vera::split(_line,',');
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
            std::vector<std::string> values = vera::split(_line,',');
            if (values.size() == 2) {
                showBBoxes = values[1] == "on";
                return true;
            }
        }
        return false;
    },
    "bboxes[,on|off]", "show/hide models bounding boxes"));
    
    _uniforms.functions["u_model"] = UniformFunction("vec3", [this](vera::Shader& _shader) {
        _shader.setUniform("u_model", m_origin.getPosition());
    },
    [this]() { return vera::toString(m_origin.getPosition(), ','); });

    _uniforms.functions["u_modelMatrix"] = UniformFunction("mat4", [this](vera::Shader& _shader) {
        _shader.setUniform("u_modelMatrix", m_origin.getTransformMatrix() );
    });
    
}

void SceneRender::addDefine(const std::string& _define, const std::string& _value) {
    m_background_shader.addDefine(_define, _value);
    m_floor_shader.addDefine(_define, _value);
}

void SceneRender::delDefine(const std::string& _define) {
    m_background_shader.delDefine(_define);
    m_floor_shader.delDefine(_define);
}

void SceneRender::printDefines() {
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
}

bool SceneRender::loadScene(Uniforms& _uniforms) {
    // Calculate the total area
    vera::BoundingBox bbox;
    for (vera::ModelsMap::iterator it = _uniforms.models.begin(); it != _uniforms.models.end(); ++it) {
        vera::addLabel( it->second->getName(), it->second, vera::LABEL_RIGHT);
        bbox.expand( it->second->getBoundingBox() );
    }

    m_area = glm::max(0.5f, glm::max(glm::length(bbox.min), glm::length(bbox.max)));
    m_origin.setPosition( -bbox.getCenter() );
    
    m_floor_height = bbox.min.y;

    // Setup light
    _uniforms.setSunPosition( glm::vec3(0.0,m_area*10.0,m_area*10.0) );
    vera::addLabel("u_light", _uniforms.lights["default"], vera::LABEL_DOWN, 30.0f);

    return true;
}

bool SceneRender::loadShaders(Uniforms& _uniforms, const std::string& _fragmentShader, const std::string& _vertexShader, bool _verbose) {
    bool rta = true;
    for (vera::ModelsMap::iterator it = _uniforms.models.begin(); it != _uniforms.models.end(); ++it) 
        if ( !it->second->setShader( _fragmentShader, _vertexShader, _verbose) )
            rta = false;

    m_background = checkBackground(_fragmentShader);
    if (m_background) {
        // Specific defines for this buffer
        m_background_shader.addDefine("BACKGROUND");
        m_background_shader.load(_fragmentShader, vera::getDefaultSrc(vera::VERT_BILLBOARD), false);
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

void SceneRender::flagChange() { m_origin.bChange = true; }
bool SceneRender::haveChange() const { return  m_origin.bChange; }
void SceneRender::unflagChange() {  m_origin.bChange = false; }

void SceneRender::render(Uniforms& _uniforms) {
    // Render Background
    renderBackground(_uniforms);

    // Begining of DEPTH for 3D 
    if (m_depth_test)
        glEnable(GL_DEPTH_TEST);

    vera::blendMode(m_blend);

    if (_uniforms.activeCamera->bChange || m_origin.bChange) {
        vera::setCamera( _uniforms.activeCamera );
        vera::applyMatrix( m_origin.getTransformMatrix() );
    }

    // TRACK_BEGIN("render:scene:floor")
    renderFloor(_uniforms, vera::getProjectionViewWorldMatrix() );
    // TRACK_END("render:scene:floor")

    vera::cullingMode(m_culling);

    for (vera::ModelsMap::iterator it = _uniforms.models.begin(); it != _uniforms.models.end(); ++it) {
        if (it->second->getShadeShader()->isLoaded() ) {
            TRACK_BEGIN("render:scene:" + it->second->getName() )

            // bind the shader
            it->second->getShadeShader()->use();

            // Update Uniforms and textures variables to the shader
            _uniforms.feedTo( it->second->getShadeShader() );

            // Pass special uniforms
            it->second->getShadeShader()->setUniform( "u_modelViewProjectionMatrix", vera::getProjectionViewWorldMatrix() );
            it->second->render();

            TRACK_END("render:scene:" + it->second->getName() )
        }
    }

    if (m_depth_test)
        glDisable(GL_DEPTH_TEST);

    if (m_blend != 0)
        vera::blendMode(vera::BLEND_ALPHA);

    if (m_culling != 0)
        glDisable(GL_CULL_FACE);
}

void SceneRender::renderShadowMap(Uniforms& _uniforms) {
    if (!m_shadows)
        return;

    // TRACK_BEGIN("shadowmap")
    bool dirty = false;
    for (vera::LightsMap::iterator lit = _uniforms.lights.begin(); lit != _uniforms.lights.end(); ++lit) {
        if (m_dynamicShadows || 
            lit->second->bChange || 
            lit->second->bUpdateShadowMap || 
            haveChange() ) {

            // Temporally move the MVP matrix from the view of the light 
            glm::mat4 mvp = lit->second->getMVPMatrix( m_origin.getTransformMatrix(), m_area );
            
            lit->second->bindShadowMap();
            
            dirty += _uniforms.models.size() == 0;
            for (vera::ModelsMap::iterator mit = _uniforms.models.begin(); mit != _uniforms.models.end(); ++mit) {
                if (mit->second->getShadowShader()->isLoaded() ) {

                    // bind the shader
                    mit->second->getShadowShader()->use();

                    // Update Uniforms and textures variables to the shader
                    _uniforms.feedTo( mit->second->getShadowShader(), false );

                    // Pass special uniforms
                    mit->second->getShadowShader()->setUniform( "u_modelViewProjectionMatrix", mvp );
                    mit->second->renderShadow();
                }
                else
                    dirty += true;
            }

            lit->second->unbindShadowMap();
        }

        lit->second->bUpdateShadowMap += dirty;
    }
    // TRACK_END("shadowmap")
}

void SceneRender::renderBackground(Uniforms& _uniforms) {
    if (m_background) {
        TRACK_BEGIN("render:scene:background")

        m_background_shader.use();

        // Update Uniforms and textures
        _uniforms.feedTo( &m_background_shader );

        vera::getBillboard()->render( &m_background_shader );

        TRACK_END("render:scene:background")
    }

    else if (_uniforms.activeCubemap) {
        if (showCubebox && _uniforms.activeCubemap->isLoaded()) {
            if (!m_cubemap_vbo) {
                m_cubemap_vbo = new vera::Vbo( vera::cubeMesh(1.0f) );
                m_cubemap_shader.load(vera::getDefaultSrc(vera::FRAG_CUBEMAP), vera::getDefaultSrc(vera::VERT_CUBEMAP), false);
            }

            glm::mat4 ori = _uniforms.activeCamera->getOrientationMatrix();

            #if defined(__EMSCRIPTEN__)
            if (vera::getXR() == vera::VR_MODE)
                ori = glm::inverse(ori);
            #endif

            m_cubemap_shader.use();
            m_cubemap_shader.setUniform("u_modelViewProjectionMatrix", _uniforms.activeCamera->getProjectionMatrix() * ori );
            m_cubemap_shader.setUniformTextureCube("u_cubeMap", _uniforms.activeCubemap, 0);

            m_cubemap_vbo->render(&m_cubemap_shader);
        }
    }
}

void SceneRender::renderFloor(Uniforms& _uniforms, const glm::mat4& _mvp, bool _lights) {
    if (m_floor_subd_target >= 0) {

        //  Floor
        if (m_floor_subd_target != m_floor_subd) {

            if (m_floor_vbo)
                delete m_floor_vbo;

            m_floor_vbo = new vera::Vbo( vera::floorMesh(m_area * 10.0f, m_floor_subd_target, m_floor_height) );
            m_floor_subd = m_floor_subd_target;

            if (!m_floor_shader.isLoaded()) 
                m_floor_shader.load(vera::getDefaultSrc(vera::FRAG_DEFAULT_SCENE), vera::getDefaultSrc(vera::VERT_DEFAULT_SCENE), false);

            m_floor_shader.addDefine("FLOOR");
            m_floor_shader.addDefine("FLOOR_SUBD", m_floor_subd);
            m_floor_shader.addDefine("FLOOR_AREA", m_area * 10.0f);
            m_floor_shader.addDefine("FLOOR_HEIGHT", m_floor_height);

            m_floor_shader.addDefine("MODEL_VERTEX_COLOR", "v_color");
            m_floor_shader.addDefine("MODEL_VERTEX_NORMAL", "v_normal");
            m_floor_shader.addDefine("MODEL_VERTEX_TEXCOORD","v_texcoord");
            
            m_floor_shader.addDefine("LIGHT_SHADOWMAP", "u_lightShadowMap");
            
            #if defined(PLATFORM_RPI)
                m_floor_shader.addDefine("LIGHT_SHADOWMAP_SIZE", "512.0");
            #else
                m_floor_shader.addDefine("LIGHT_SHADOWMAP_SIZE", "1024.0");
            #endif
        }

        if (m_floor_vbo) {
            m_floor_shader.use();
            _uniforms.feedTo( &m_floor_shader, _lights );
            m_floor_shader.setUniform("u_modelViewProjectionMatrix", _mvp );
            m_floor_vbo->render( &m_floor_shader );
        }

    }
}

void SceneRender::renderDebug(Uniforms& _uniforms) {
    glEnable(GL_DEPTH_TEST);
    vera::blendMode(vera::BLEND_ALPHA);

    vera::Shader* fill = vera::getFillShader();
    
    // Draw Bounding boxes
    if (showBBoxes) {
        vera::strokeWeight(3.0f);

        vera::stroke(glm::vec3(1.0f, 0.0f, 0.0f));
        for (vera::ModelsMap::iterator it = _uniforms.models.begin(); it != _uniforms.models.end(); ++it)
            vera::model( it->second->getVboBbox() );

        vera::fill(.8f);
        vera::textSize(24.0f);
        vera::labels();
    }
    
    // Axis
    if (showAxis) {
        if (m_axis_vbo == nullptr)
            m_axis_vbo = new vera::Vbo( vera::axisMesh(_uniforms.activeCamera->getFarClip(), m_floor_height) );

        vera::strokeWeight(2.0f);
        vera::stroke( glm::vec4(1.0f) );
        vera::model( m_axis_vbo );
    }
    
    // Grid
    if (showGrid) {
        if (m_grid_vbo == nullptr)
            m_grid_vbo = new vera::Vbo( vera::gridMesh(_uniforms.activeCamera->getFarClip(), _uniforms.activeCamera->getFarClip() / 20.0, m_floor_height) );

        vera::strokeWeight(1.0f);
        vera::stroke( glm::vec4(0.5f) );
        vera::model( m_grid_vbo );
    }

    // Light
    {
        if (!m_lightUI_shader.isLoaded())
            m_lightUI_shader.load(vera::getDefaultSrc(vera::FRAG_LIGHT), vera::getDefaultSrc(vera::VERT_LIGHT), false);

        if (m_lightUI_vbo == nullptr)
            m_lightUI_vbo = new vera::Vbo( vera::rectMesh(0.0,0.0,0.0,0.0) );

        m_lightUI_shader.use();
        m_lightUI_shader.setUniform("u_scale", 12.0f, 12.0f);
        m_lightUI_shader.setUniform("u_viewMatrix", _uniforms.activeCamera->getViewMatrix());
        m_lightUI_shader.setUniform("u_modelViewProjectionMatrix", vera::getProjectionViewWorldMatrix() );

        for (vera::LightsMap::iterator it = _uniforms.lights.begin(); it != _uniforms.lights.end(); ++it) {
            m_lightUI_shader.setUniform("u_translate", it->second->getPosition());
            m_lightUI_shader.setUniform("u_color", glm::vec4(it->second->color, 1.0));
            m_lightUI_vbo->render( &m_lightUI_shader );
        }
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
}
