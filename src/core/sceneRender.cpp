
#include "sceneRender.h"

#ifndef PLATFORM_WINDOWS
#include <unistd.h>
#endif

#include <sys/stat.h>
#include <random>

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


#if defined(DEBUG)

#define TRACK_BEGIN(A) if (_uniforms.tracker.isRunning()) _uniforms.tracker.begin(A); 
#define TRACK_END(A) if (_uniforms.tracker.isRunning()) _uniforms.tracker.end(A); 

#else 

#define TRACK_BEGIN(A) 
#define TRACK_END(A)

#endif

SceneRender::SceneRender(): 
    // Debug State
    showGrid(false), showAxis(false), showBBoxes(false), showCubebox(false), 
    // Camera.
    m_blend(vera::BLEND_ALPHA), m_culling(vera::CULL_NONE), m_depth_test(true),
    // Light
    dynamicShadows(false), m_shadows(false),
    // Background
    m_background(false), 
    // Floor
    m_floor_height(0.0), m_floor_subd_target(-1), m_floor_subd(-1),

    m_buffers_total(0), m_commands_loaded(false), m_uniforms_loaded(false)
    {
    m_origin.setPosition(glm::vec3(0.0));
}

SceneRender::~SceneRender() {
}

void SceneRender::commandsInit(CommandList& _commands, Uniforms& _uniforms) {
    if (!m_commands_loaded) {

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
                if (values.size() == 2) {
                    for (vera::ModelsMap::iterator it = _uniforms.models.begin(); it != _uniforms.models.end(); ++it)
                        if (it->second->getName() == values[1]) {
                            it->second->printVboInfo();
                            std::cout << "Defines:" << std::endl;
                            it->second->printDefines();
                            std::cout << "Position:" << std::endl;
                            glm::vec3 pos = it->second->getPosition();
                            std::cout << pos.x << ',' << pos.y << ',' << pos.z << std::endl;
                            return true;
                        }
                }
                else if (values.size() == 5) {
                    for (vera::ModelsMap::iterator it = _uniforms.models.begin(); it != _uniforms.models.end(); ++it)
                        if (it->second->getName() == values[1]) {
                            it->second->setPosition( glm::vec3(vera::toFloat(values[2]), vera::toFloat(values[3]), vera::toFloat(values[4])) );
                            return true;
                        }
                }
            }
            return false;
        },
        "models", "print all the names of the models"));

        _commands.push_back(Command("origin", [&](const std::string& _line){ 
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
        "origin[,<x>,<y>,<z>]", "get or set the origin position."));

        _commands.push_back(Command("material", [&](const std::string& _line){ 
            if (_line == "materials") {
                for (vera::MaterialsMap::iterator it = _uniforms.materials.begin(); it != _uniforms.materials.end(); it++)
                    std::cout << it->second->name << std::endl;
                return true;
            }
            else {
                std::vector<std::string> values = vera::split(_line,',');
                if (values.size() == 2 && values[0] == "material") {
                    for (vera::MaterialsMap::iterator it = _uniforms.materials.begin(); it != _uniforms.materials.end(); it++) {
                        if (it->second->name == values[1]) {
                            it->second->printDefines();
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
                std::string rta = dynamicShadows ? "on" : "off";
                std::cout <<  rta << std::endl; 
                return true;
            }
            else {
                std::vector<std::string> values = vera::split(_line,',');
                if (values.size() == 2) {
                    dynamicShadows = (values[1] == "on");
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
        
        _commands.push_back(Command("floor", [&](const std::string& _line) {
            std::vector<std::string> values = vera::split(_line,',');

            if (_line == "floor") {
                std::string rta = m_floor_subd > 0 ? vera::toString(m_floor_subd) : "off";
                std::cout << rta << std::endl; 
                return true;
            }
            else {
                if (values.size() == 2) {
                    if (values[1] == "toggle")
                        values[1] = m_floor_subd_target >= 0 ? "off" : "on";

                    if (values[1] == "off")
                        m_floor_subd_target = -1;
                    else if (values[1] == "on") {
                        if (m_floor_subd_target == -1)
                            m_floor_subd_target = 0;
                    }
                    else
                        m_floor_subd_target = vera::toInt(values[1]);
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
                    if (values[1] == "toggle")
                        values[1] = showGrid ? "off" : "on";

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
                    if (values[1] == "toggle")
                        values[1] = showAxis ? "off" : "on";

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
                    if (values[1] == "toggle")
                        values[1] = showBBoxes ? "off" : "on";

                    showBBoxes = values[1] == "on";
                    return true;
                }
            }
            return false;
        },
        "bboxes[,on|off]", "show/hide models bounding boxes"));
        m_commands_loaded = true;
    }
}

void SceneRender::uniformsInit(Uniforms& _uniforms) {
    if (!m_uniforms_loaded) {
        // ADD UNIFORMS
        //
        _uniforms.functions["u_area"] = UniformFunction("float", [this](vera::Shader& _shader) {
            _shader.setUniform("u_area", m_area);
        },
        [this]() { return vera::toString(m_area); });

        _uniforms.functions["u_scene"] = UniformFunction("sampler2D", [this](vera::Shader& _shader) {
            if (renderFbo.getTextureId())
                _shader.setUniformTexture("u_scene", &renderFbo, _shader.textureIndex++ );
        });

        _uniforms.functions["u_sceneDepth"] = UniformFunction("sampler2D", [this](vera::Shader& _shader) {
            if (renderFbo.getTextureId())
                _shader.setUniformDepthTexture("u_sceneDepth", &renderFbo, _shader.textureIndex++ );
        });

        _uniforms.functions["u_sceneNormal"] = UniformFunction("sampler2D", [this](vera::Shader& _shader) {
            if (normalFbo.getTextureId())
                _shader.setUniformTexture("u_sceneNormal", &normalFbo, _shader.textureIndex++ );
        });

        _uniforms.functions["u_scenePosition"] = UniformFunction("sampler2D", [this](vera::Shader& _shader) {
            if (positionFbo.getTextureId())
                _shader.setUniformTexture("u_scenePosition", &positionFbo, _shader.textureIndex++ );
        });

        // SSAO data (https://learnopengl.com/Advanced-Lighting/SSAO)
        //
        std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between [0.0, 1.0]
        std::default_random_engine generator;

        for (size_t i = 0; i < 64; ++i) {
            glm::vec3 sample(   randomFloats(generator) * 2.0 - 1.0, 
                                randomFloats(generator) * 2.0 - 1.0, 
                                randomFloats(generator) );

            sample = glm::normalize(sample);
            float scale = (float)i / 64.0;
            scale   = vera::lerp(0.1f, 1.0f, scale * scale);
            sample *= scale;
            m_ssaoSamples[i] = sample;
        }

        _uniforms.functions["u_ssaoSamples"] = UniformFunction("vec3", [this](vera::Shader& _shader) {
            _shader.setUniform("u_ssaoSamples", m_ssaoSamples, 64 );
        });

        for (size_t i = 0; i < 16; i++) {
            m_ssaoNoise[i] = glm::vec3( randomFloats(generator) * 2.0 - 1.0, 
                                        randomFloats(generator) * 2.0 - 1.0, 
                                        0.0f );
        }

        _uniforms.functions["u_ssaoNoise"] = UniformFunction("vec3", [this](vera::Shader& _shader) {
            _shader.setUniform("u_ssaoNoise", m_ssaoNoise, 16 );
        });
        m_uniforms_loaded = false;
    }
}

void SceneRender::addDefine(const std::string& _define, const std::string& _value) {
    m_background_shader.addDefine(_define, _value);
    m_floor.addDefine(_define, _value);
}

void SceneRender::delDefine(const std::string& _define) {
    m_background_shader.delDefine(_define);
    m_floor.delDefine(_define);
}

void SceneRender::flagChange() { 
    m_origin.bChange = true; 
}
bool SceneRender::haveChange() const { 
    return m_origin.bChange; 
}
void SceneRender::unflagChange() {  
    m_origin.bChange = false; 
}

void SceneRender::printDefines() {
    if (m_background) {
        std::cout << "." << std::endl;
        std::cout << "| BACKGROUND" << std::endl;
        std::cout << "+------------- " << std::endl;
        m_background_shader.printDefines();
    }

    if (m_floor_subd >= 0) {
        std::cout << "." << std::endl;
        std::cout << "| FLOOR" << std::endl;
        std::cout << "+------------- " << std::endl;
        m_floor.getShader()->printDefines();
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
    // m_origin.setPosition( -bbox.getCenter() );
    
    // Floor
    m_floor_height = bbox.min.y;
    m_floor.addDefine("FLOOR");
    m_floor.addDefine("MODEL_VERTEX_COLOR", "v_color");
    m_floor.addDefine("MODEL_VERTEX_NORMAL", "v_normal");
    m_floor.addDefine("MODEL_VERTEX_TEXCOORD","v_texcoord");
    m_floor.setShader(vera::getDefaultSrc(vera::FRAG_DEFAULT_SCENE), vera::getDefaultSrc(vera::VERT_DEFAULT_SCENE));

    // Cubemap
    m_cubemap_shader.setSource(vera::getDefaultSrc(vera::FRAG_CUBEMAP), vera::getDefaultSrc(vera::VERT_CUBEMAP));

    // Light
    #if !defined(PYTHON_RENDER)
    _uniforms.setSunPosition( glm::vec3(0.0,m_area*10.0,m_area*10.0) );
    #endif
    vera::addLabel("u_light", _uniforms.lights["default"], vera::LABEL_DOWN, 30.0f);
    m_lightUI_shader.setSource(vera::getDefaultSrc(vera::FRAG_LIGHT), vera::getDefaultSrc(vera::VERT_LIGHT));

    return true;
}

bool SceneRender::clearScene() {
    m_floor.clear();
    m_floor_subd = -1;
    m_floor_height = 0.0;
    m_origin.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    return true;
}

void SceneRender::setShaders(Uniforms& _uniforms, const std::string& _fragmentShader, const std::string& _vertexShader) {
    // bool something_fail = false;

    // Background
    m_background = checkBackground(_fragmentShader);
    if (m_background) {
        // Specific defines for this buffer
        m_background_shader.addDefine("BACKGROUND");
        m_background_shader.setSource(_fragmentShader, vera::getDefaultSrc(vera::VERT_BILLBOARD));
        m_background_shader.addDefine("GLSLVIEWER", vera::toString(GLSLVIEWER_VERSION_MAJOR) + vera::toString(GLSLVIEWER_VERSION_MINOR) + vera::toString(GLSLVIEWER_VERSION_PATCH) );
    }

    m_shadows = findId(_fragmentShader, "u_lightShadowMap;");
    bool position_buffer = findId(_fragmentShader, "u_scenePosition;");// _uniforms.functions["u_scenePosition"].present;
    bool normal_buffer = findId(_fragmentShader, "u_sceneNormal;"); //_uniforms.functions["u_sceneNormal"].present; 
    m_buffers_total = std::max( countSceneBuffers(_vertexShader), 
                                countSceneBuffers(_fragmentShader) );

    for (vera::ModelsMap::iterator it = _uniforms.models.begin(); it != _uniforms.models.end(); ++it) {
        it->second->setShader( _fragmentShader, _vertexShader);

        if (m_shadows) {}
            it->second->setBufferShader("shadow", vera::getDefaultSrc(vera::FRAG_ERROR), _vertexShader);

        if (position_buffer)
            it->second->setBufferShader("position", vera::getDefaultSrc(vera::FRAG_POSITION), _vertexShader);
        
        if (normal_buffer)
            it->second->setBufferShader("normal", vera::getDefaultSrc(vera::FRAG_NORMAL), _vertexShader);

        for (size_t i = 0; i < m_buffers_total; i++) {
            std::string bufferName = "u_sceneBuffer" + vera::toString(i);
            it->second->setBufferShader(bufferName, _fragmentShader, _vertexShader);
            it->second->getBufferShader(bufferName)->delDefine("FLOOR");
            it->second->getBufferShader(bufferName)->addDefine("SCENE_BUFFER_" + vera::toString(i));
        }
    }

    // Floor
    bool thereIsFloorDefine = checkFloor(_fragmentShader) || checkFloor(_vertexShader);
    if (thereIsFloorDefine) {
        m_floor.setShader(_fragmentShader, _vertexShader);

        if (m_floor_subd == -1)
            m_floor_subd_target = 0;

        if (m_shadows) 
            m_floor.setBufferShader("shadow", vera::getDefaultSrc(vera::FRAG_ERROR), _vertexShader);

        if (position_buffer)
            m_floor.setBufferShader("position", vera::getDefaultSrc(vera::FRAG_POSITION), _vertexShader);
        
        if (normal_buffer)
            m_floor.setBufferShader("normal", vera::getDefaultSrc(vera::FRAG_NORMAL), _vertexShader);

        for (size_t i = 0; i < m_buffers_total; i++) {
            std::string bufferName = "u_sceneBuffer" + vera::toString(i);
            m_floor.setBufferShader(bufferName, _fragmentShader, _vertexShader);
            m_floor.getBufferShader(bufferName)->addDefine("FLOOR");
            m_floor.getBufferShader(bufferName)->addDefine("SCENE_BUFFER_" + vera::toString(i));
        }
    }

    return;
}

void SceneRender::updateBuffers(Uniforms& _uniforms, int _width, int _height) {
    vera::FboType type = _uniforms.functions["u_sceneDepth"].present ? vera::COLOR_DEPTH_TEXTURES : vera::COLOR_TEXTURE_DEPTH_BUFFER;

    if (!renderFbo.isAllocated() ||
        renderFbo.getType() != type || 
        renderFbo.getWidth() != _width || renderFbo.getHeight() != _height )
        renderFbo.allocate(_width, _height, type);

    if (_uniforms.functions["u_sceneNormal"].present && 
        (   !normalFbo.isAllocated() || 
            normalFbo.getWidth() != _width || normalFbo.getHeight() != _height ) )
        normalFbo.allocate(_width, _height, vera::GBUFFER_TEXTURE);

    
    if (_uniforms.functions["u_scenePosition"].present && 
        (   !positionFbo.isAllocated() || 
            positionFbo.getWidth() != _width || positionFbo.getHeight() != _height ) )
        positionFbo.allocate(_width, _height, vera::GBUFFER_TEXTURE);

    for (size_t i = 0; i < buffersFbo.size(); i++)
        if (buffersFbo[i]->isAllocated() ||
            buffersFbo[i]->getWidth() != _width || buffersFbo[i]->getHeight() )
            buffersFbo[i]->allocate(_width, _height, vera::GBUFFER_TEXTURE);
}

void SceneRender::printBuffers() {
    for (size_t i = 0; i < buffersFbo.size(); i++)
        std::cout << "uniform sampler2D u_sceneBuffer" << i << ";" << std::endl;
}

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

    TRACK_BEGIN("render:scene:floor")
    renderFloor(_uniforms );
    TRACK_END("render:scene:floor")

    vera::cullingMode(m_culling);

    for (vera::ModelsMap::iterator it = _uniforms.models.begin(); it != _uniforms.models.end(); ++it) {
        TRACK_BEGIN("render:scene:" + it->second->getName() )

        // bind the shader
        it->second->getShader()->use();

        // Update Uniforms and textures variables to the shader
        _uniforms.feedTo( it->second->getShader() );

        // Pass special uniforms
        it->second->getShader()->setUniform( "u_modelViewProjectionMatrix", vera::getProjectionViewWorldMatrix() * it->second->getTransformMatrix() );
        it->second->getShader()->setUniform( "u_modelMatrix", m_origin.getTransformMatrix() * it->second->getTransformMatrix() );
        it->second->getShader()->setUniform( "u_model", m_origin.getPosition() + m_floor.getPosition() );
        for (size_t i = 0; i < buffersFbo.size(); i++)
            it->second->getShader()->setUniformTexture("u_sceneBuffer" + vera::toString(i), buffersFbo[i], it->second->getShader()->textureIndex++);

        it->second->render();

        TRACK_END("render:scene:" + it->second->getName() )
    }

    if (m_depth_test)
        glDisable(GL_DEPTH_TEST);

    if (m_blend != 0)
        vera::blendMode(vera::BLEND_ALPHA);

    if (m_culling != 0)
        glDisable(GL_CULL_FACE);
}

void SceneRender::renderNormalBuffer(Uniforms& _uniforms) {
    if (!normalFbo.isAllocated())
        return;

    normalFbo.bind();

    // Begining of DEPTH for 3D 
    if (m_depth_test)
        glEnable(GL_DEPTH_TEST);

    if (_uniforms.activeCamera->bChange || m_origin.bChange) {
        vera::setCamera( _uniforms.activeCamera );
        vera::applyMatrix( m_origin.getTransformMatrix() );
    }

    vera::Shader* normalShader = nullptr;
    if (m_floor_subd_target >= 0) {
        normalShader = m_floor.getBufferShader("normal");
        if (normalShader != nullptr) {
            TRACK_BEGIN("render:sceneNormal:floor")
            normalShader->use();
            _uniforms.feedTo( normalShader, false );
            normalShader->setUniform("u_modelViewProjectionMatrix", vera::getProjectionViewWorldMatrix() * m_floor.getTransformMatrix());
            normalShader->setUniform("u_model", m_origin.getPosition() + m_floor.getPosition() );
            normalShader->setUniform("u_modelMatrix", m_origin.getTransformMatrix() * m_floor.getTransformMatrix() );
            m_floor.render(normalShader);
            TRACK_END("render:sceneNormal:floor")
        } 
    }

    vera::cullingMode(m_culling);

    for (vera::ModelsMap::iterator it = _uniforms.models.begin(); it != _uniforms.models.end(); ++it) {
        normalShader = it->second->getBufferShader("normal");
        if (normalShader != nullptr) {
            TRACK_BEGIN("render:sceneNormal:" + it->second->getName() )

            // bind the shader
            normalShader->use();

            // Update Uniforms and textures variables to the shader
            _uniforms.feedTo( normalShader, false );

            // Pass special uniforms
            normalShader->setUniform( "u_modelViewProjectionMatrix", vera::getProjectionViewWorldMatrix() * it->second->getTransformMatrix());
            normalShader->setUniform( "u_model", m_origin.getPosition() + it->second->getPosition() );
            normalShader->setUniform( "u_modelMatrix", m_origin.getTransformMatrix() * it->second->getTransformMatrix() );
            it->second->render(normalShader);

            TRACK_END("render:sceneNormal:" + it->second->getName() )
        }
    }

    if (m_depth_test)
        glDisable(GL_DEPTH_TEST);

    if (m_culling != 0)
        glDisable(GL_CULL_FACE);

    normalFbo.unbind();
}

void SceneRender::renderPositionBuffer(Uniforms& _uniforms) {
    if (!positionFbo.isAllocated())
        return;

    positionFbo.bind();

    // Begining of DEPTH for 3D 
    if (m_depth_test)
        glEnable(GL_DEPTH_TEST);

    if (_uniforms.activeCamera->bChange || m_origin.bChange) {
        vera::setCamera( _uniforms.activeCamera );
        vera::applyMatrix( m_origin.getTransformMatrix() );
    }

    vera::Shader* positionShader = nullptr;
    if (m_floor_subd_target >= 0) {
        positionShader = m_floor.getBufferShader("position");
        if (positionShader != nullptr) {
            TRACK_BEGIN("render:scenePosition:floor")
            positionShader->use();
            _uniforms.feedTo( positionShader, false );
            positionShader->setUniform("u_modelViewProjectionMatrix", vera::getProjectionViewWorldMatrix() *  m_floor.getTransformMatrix() );
            positionShader->setUniform("u_model", m_origin.getPosition() + m_floor.getPosition() );
            positionShader->setUniform("u_modelMatrix", m_origin.getTransformMatrix() * m_floor.getTransformMatrix() );
            m_floor.render(positionShader);
            TRACK_END("render:scenePosition:floor")
        } 
    }

    vera::cullingMode(m_culling);

    for (vera::ModelsMap::iterator it = _uniforms.models.begin(); it != _uniforms.models.end(); ++it) {
        positionShader = it->second->getBufferShader("position");
        if (positionShader != nullptr) {
            TRACK_BEGIN("render:scenePosition:" + it->second->getName() )

            // bind the shader
            positionShader->use();

            // Update Uniforms and textures variables to the shader
            _uniforms.feedTo( positionShader, false );

            // Pass special uniforms
            positionShader->setUniform( "u_modelViewProjectionMatrix", vera::getProjectionViewWorldMatrix() *  it->second->getTransformMatrix() );
            positionShader->setUniform( "u_model", m_origin.getPosition() + it->second->getPosition() );
            positionShader->setUniform( "u_modelMatrix", m_origin.getTransformMatrix() * it->second->getTransformMatrix() );
            it->second->render(positionShader);

            TRACK_END("render:scenePosition:" + it->second->getName() )
        }
    }

    if (m_depth_test)
        glDisable(GL_DEPTH_TEST);

    if (m_culling != 0)
        glDisable(GL_CULL_FACE);

    positionFbo.unbind();
}

void SceneRender::renderBuffers(Uniforms& _uniforms) {
    if ( m_buffers_total != buffersFbo.size() ) {
        for (size_t i = 0; i < buffersFbo.size(); i++)
            delete buffersFbo[i];
        buffersFbo.clear();
        
        for (size_t i = 0; i < m_buffers_total; i++) {
            buffersFbo.push_back( new vera::Fbo() );

            // glm::vec2 size = glm::vec2(vera::getWindowWidth(), vera::getWindowHeight());
            // buffersFbo[i].fixed = getBufferSize(_fragmentShader, "u_sceneBuffer" + vera::toString(i), size);
            buffersFbo[i]->allocate(vera::getWindowWidth(), vera::getWindowHeight(), vera::GBUFFER_TEXTURE);
        }
    }

    vera::Shader* bufferShader = nullptr;
    for (size_t i = 0; i < buffersFbo.size(); i++) {
        if (!buffersFbo[i]->isAllocated())
            continue;;

        buffersFbo[i]->bind();
        std::string bufferName = "u_sceneBuffer" + vera::toString(i);

        // Begining of DEPTH for 3D 
        if (m_depth_test)
            glEnable(GL_DEPTH_TEST);

        // if (_uniforms.activeCamera->bChange)
        //     vera::setCamera( _uniforms.activeCamera );

        if (_uniforms.activeCamera->bChange || m_origin.bChange) {
            vera::setCamera( _uniforms.activeCamera );
            vera::applyMatrix( m_origin.getTransformMatrix() );
        }

        if (m_floor_subd_target >= 0) {
            bufferShader = m_floor.getBufferShader(bufferName);
            if (bufferShader != nullptr) {
                    TRACK_BEGIN("render:"+bufferName+":floor")
                    bufferShader->use();
                    _uniforms.feedTo( bufferShader, false );
                    bufferShader->setUniform( "u_modelViewProjectionMatrix", vera::getProjectionViewWorldMatrix() * m_floor.getTransformMatrix() );
                    bufferShader->setUniform( "u_model", m_origin.getPosition() + m_floor.getPosition() );
                    bufferShader->setUniform( "u_modelMatrix", m_origin.getTransformMatrix() * m_floor.getTransformMatrix() );
                    m_floor.render(bufferShader);
                    TRACK_END("render:"+bufferName+":floor")
                }
        }

        vera::cullingMode(m_culling);

        for (vera::ModelsMap::iterator it = _uniforms.models.begin(); it != _uniforms.models.end(); ++it) {
            bufferShader = it->second->getBufferShader(bufferName);

            if (bufferShader != nullptr) {
                TRACK_BEGIN("render:" + bufferName + ":" + it->second->getName())

                // bind the shader
                bufferShader->use();

                // Update Uniforms and textures variables to the shader
                _uniforms.feedTo( bufferShader, false );

                // Pass special uniforms
                bufferShader->setUniform( "u_modelViewProjectionMatrix", vera::getProjectionViewWorldMatrix() * it->second->getTransformMatrix() );
                bufferShader->setUniform( "u_model", m_origin.getPosition() + it->second->getPosition() );
                bufferShader->setUniform( "u_modelMatrix", m_origin.getTransformMatrix() * it->second->getTransformMatrix() );
                it->second->render(bufferShader);

                TRACK_END("render:" + bufferName + ":" + it->second->getName())
            }
        }

        if (m_depth_test)
            glDisable(GL_DEPTH_TEST);

        if (m_culling != 0)
            glDisable(GL_CULL_FACE);

        buffersFbo[i]->unbind();
    }
}

void SceneRender::renderShadowMap(Uniforms& _uniforms) {
    if (!m_shadows)
        return;

    TRACK_BEGIN("render:scene:shadowmap")
    vera::Shader* shadowShader = nullptr;
    for (vera::LightsMap::iterator lit = _uniforms.lights.begin(); lit != _uniforms.lights.end(); ++lit) {
        if (dynamicShadows ||  lit->second->bChange || haveChange() ) {
            // Temporally move the MVP matrix from the view of the light 
            glm::mat4 m = m_origin.getTransformMatrix();
            // glm::mat4 p = lit->second->getProjectionMatrix();
            // glm::mat4 v = lit->second->getViewMatrix();
            
            lit->second->bindShadowMap();

            shadowShader = m_floor.getBufferShader("shadow");
            if (m_floor.getVbo() && shadowShader != nullptr) {
                TRACK_BEGIN("render:scene:shadowmap:floor")
                shadowShader->use();
                _uniforms.feedTo( shadowShader, false );
                shadowShader->setUniform( "u_modelViewProjectionMatrix", lit->second->getMVPMatrix( m_origin.getTransformMatrix() * m_floor.getTransformMatrix(), m_area ) );
                shadowShader->setUniform( "u_projectionMatrix", lit->second->getProjectionMatrix() );
                shadowShader->setUniform( "u_viewMatrix", lit->second->getViewMatrix() );
                shadowShader->setUniform( "u_modelMatrix", m_origin.getTransformMatrix() * m_floor.getTransformMatrix() );
                shadowShader->setUniform( "u_model", m_origin.getPosition() + m_floor.getPosition() );
                m_floor.render(shadowShader);
                TRACK_END("render:scene:shadowmap:floor")
            }

            for (vera::ModelsMap::iterator mit = _uniforms.models.begin(); mit != _uniforms.models.end(); ++mit) {
                shadowShader = mit->second->getBufferShader("shadow");
                if (shadowShader != nullptr) {
                    TRACK_BEGIN("render:scene:shadowmap:" + mit->second->getName())

                    // bind the shader
                    shadowShader->use();

                    // Update Uniforms and textures variables to the shader
                    _uniforms.feedTo( shadowShader, false );

                    // Pass special uniforms
                    shadowShader->setUniform( "u_modelViewProjectionMatrix", lit->second->getMVPMatrix( m_origin.getTransformMatrix() * mit->second->getTransformMatrix(), m_area ) );
                    shadowShader->setUniform( "u_projectionMatrix", lit->second->getProjectionMatrix() );
                    shadowShader->setUniform( "u_viewMatrix", lit->second->getViewMatrix() );
                    shadowShader->setUniform( "u_modelMatrix", m_origin.getTransformMatrix() * mit->second->getTransformMatrix() );
                    shadowShader->setUniform( "u_model", m_origin.getPosition() + mit->second->getPosition() );
                    mit->second->render(shadowShader);

                    TRACK_END("render:scene:shadowmap:" + mit->second->getName())
                }
            }

            lit->second->unbindShadowMap();
        }
    }
    TRACK_END("shadowmap")
}

void SceneRender::renderBackground(Uniforms& _uniforms) {
    if (m_background) {
        TRACK_BEGIN("render:scene:background")
        glDisable(GL_DEPTH_TEST);

        m_background_shader.use();
        m_background_shader.setUniform("u_modelMatrix", glm::mat4(1.0));
        m_background_shader.setUniform("u_viewMatrix", glm::mat4(1.0));
        // m_background_shader.setUniform("u_projectionMatrix", glm::mat4(1.0));
        // m_background_shader.setUniform("u_modelViewProjectionMatrix", glm::mat4(1.0));

        // Update Uniforms and textures
        _uniforms.feedTo( &m_background_shader );

        vera::getBillboard()->render( &m_background_shader );

        TRACK_END("render:scene:background")
    }

    else if (_uniforms.activeCubemap) {
        if (showCubebox && _uniforms.activeCubemap->loaded()) {
            TRACK_BEGIN("render:scene:cubemap")

            glDisable(GL_DEPTH_TEST);

            if (!m_cubemap_vbo)
                m_cubemap_vbo = std::unique_ptr<vera::Vbo>(new vera::Vbo( vera::cubeMesh(1.0f) ));

            glm::mat4 ori = _uniforms.activeCamera->getOrientationMatrix();

            #if defined(__EMSCRIPTEN__)
            if (vera::getXR() == vera::VR_MODE)
                ori = glm::inverse(ori);
            #endif

            m_cubemap_shader.use();
            m_cubemap_shader.setUniform("u_modelViewProjectionMatrix", _uniforms.activeCamera->getProjectionMatrix() * ori);
            m_cubemap_shader.setUniform("u_modelMatrix", glm::mat4(1.0));
            m_cubemap_shader.setUniform("u_viewMatrix", ori);
            m_cubemap_shader.setUniform("u_projectionMatrix", _uniforms.activeCamera->getProjectionMatrix());
            m_cubemap_shader.setUniformTextureCube("u_cubeMap", _uniforms.activeCubemap, 0);
            m_cubemap_vbo->render(&m_cubemap_shader);

            TRACK_END("render:scene:cubemap")
        }
    }
}

void SceneRender::renderFloor(Uniforms& _uniforms, bool _lights) {
    if (m_floor_subd_target >= 0) {
        //  Floor
        if (m_floor_subd_target != m_floor_subd) {
            m_floor.setGeom( vera::floorMesh(m_area * 10.0f, m_floor_subd_target, m_floor_height) );
            m_floor_subd = m_floor_subd_target;
            m_floor.addDefine("FLOOR_SUBD", vera::toString(m_floor_subd) );
            m_floor.addDefine("FLOOR_AREA", vera::toString(m_area * 10.0f) );
            m_floor.addDefine("FLOOR_HEIGHT", vera::toString(m_floor_height) );
        }

        if (m_floor.getVbo()) {
            m_floor.getShader()->use();
            _uniforms.feedTo( m_floor.getShader(), _lights );

            m_floor.getShader()->setUniform("u_modelViewProjectionMatrix", vera::getProjectionViewWorldMatrix() * m_floor.getTransformMatrix() );
            m_floor.getShader()->setUniform("u_modelMatrix", m_origin.getTransformMatrix() * m_floor.getTransformMatrix() );
            m_floor.getShader()->setUniform("u_model", m_origin.getPosition() + m_floor.getPosition() );
            // for (size_t i = 0; i < buffersFbo.size(); i++)
            //     m_floor.getShader()->setUniformTexture("u_sceneBuffer" + vera::toString(i), &(buffersFbo[i]), m_floor.getShader()->textureIndex++);
            
            m_floor.render();
        }
    }
}

void SceneRender::renderDebug(Uniforms& _uniforms) {
    glEnable(GL_DEPTH_TEST);
    vera::blendMode(vera::BLEND_ALPHA);

    vera::Shader* fill = vera::getFillShader();
    
    // Draw Bounding boxes
    if (showBBoxes) {
        #if defined(PYTHON_RENDER)
        vera::strokeWeight(1.0f);
        #else
        vera::strokeWeight(3.0f);
        #endif

        vera::stroke(glm::vec3(1.0f, 0.0f, 0.0f));
        for (vera::ModelsMap::iterator it = _uniforms.models.begin(); it != _uniforms.models.end(); ++it) {
            vera::applyMatrix( it->second->getTransformMatrix() );
            vera::model( it->second->getVboBbox() );
        }

        vera::resetMatrix();
        vera::fill(.8f);
        vera::textSize(24.0f);
        vera::labels();
    }
    
    // Axis
    if (showAxis) {
        if (m_axis_vbo == nullptr)
            m_axis_vbo = std::unique_ptr<vera::Vbo>(new vera::Vbo( vera::axisMesh(_uniforms.activeCamera->getFarClip(), m_floor_height) ));

        vera::strokeWeight(2.0f);
        vera::stroke( glm::vec4(1.0f) );
        vera::model( m_axis_vbo.get() );
    }
    
    // Grid
    if (showGrid) {
        if (m_grid_vbo == nullptr)
            m_grid_vbo = std::unique_ptr<vera::Vbo>(new vera::Vbo( vera::gridMesh(_uniforms.activeCamera->getFarClip(), _uniforms.activeCamera->getFarClip() / 20.0, m_floor_height) ));

        vera::strokeWeight(1.0f);
        vera::stroke( glm::vec4(0.5f) );
        vera::model( m_grid_vbo.get() );
    }

    // Light
    {
        if (m_lightUI_vbo == nullptr)
            m_lightUI_vbo = std::unique_ptr<vera::Vbo>(new vera::Vbo( vera::rectMesh(0.0,0.0,0.0,0.0) ));

        m_lightUI_shader.use();
        m_lightUI_shader.setUniform("u_scale", 2.0f, 2.0f);
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
