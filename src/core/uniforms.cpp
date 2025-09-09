#include "uniforms.h"

#include <regex>
#include <limits>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "tools/text.h"
#include "vera/ops/fs.h"
#include "vera/ops/draw.h"
#include "vera/ops/string.h"
#include "vera/xr/xr.h"


std::string UniformData::getType() {
    if (size == 1) return (bInt ? "int" : "float");
    else return (bInt ? "ivec" : "vec") + vera::toString(size); 
}

void UniformData::set(const UniformValue &_value, size_t _size, bool _int, bool _queue) {
    bInt = _int;
    size = _size;

    if (_queue && change)
        queue.push( _value );
    else
        value = _value;
    
    change = true;
}

void UniformData::parse(const std::vector<std::string>& _command, size_t _start, bool _queue) {;
    UniformValue candidate;
    for (size_t i = _start; i < _command.size() && i < 5; i++) 
        candidate[i-_start] = vera::toFloat(_command[i]);

    set(candidate, _command.size() - _start, false, _queue);
}

std::string UniformData::print() {
    std::string rta = "";
    for (size_t i = 0; i < size; i++) {
        rta += vera::toString(value[i]);
        if (i < size - 1)
            rta += ",";
    }
    return rta;
}

bool UniformData::check() {
    if (queue.empty())
        change = false;
    else {
        value = queue.front();
        queue.pop();
        change = true;
    }
    return change;
}

UniformFunction::UniformFunction() {
    type = "-undefined-";
}

UniformFunction::UniformFunction(const std::string &_type) {
    type = _type;
}

UniformFunction::UniformFunction(const std::string &_type, std::function<void(vera::Shader&)> _assign) {
    type = _type;
    assign = _assign;
}

UniformFunction::UniformFunction(const std::string &_type, std::function<void(vera::Shader&)> _assign, std::function<std::string()> _print) {
    type = _type;
    assign = _assign;
    print = _print;
}


Uniforms::Uniforms() : m_frame(0), m_play(true) {

    activeCubemap = nullptr;

    // IBL
    //
    functions["u_iblLuminance"] = UniformFunction("float", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_iblLuminance", float(30000.0 * activeCamera->getExposure()));
    },
    [this]() { 
        if (activeCamera)
            return vera::toString(30000.0f * activeCamera->getExposure());
        return std::string("");
    });
    
    // CAMERA UNIFORMS
    //
    functions["u_camera"] = UniformFunction("vec3", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_camera", - (activeCamera->getPosition()) );
    },
    [this]() {
        if (activeCamera)
            return vera::toString(-activeCamera->getPosition(), ',');
        return std::string("");
    });

    functions["u_cameraDistance"] = UniformFunction("float", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_cameraDistance", activeCamera->getDistance());
    },
    [this]() { 
        if (activeCamera)
            return vera::toString(activeCamera->getDistance());
        return std::string("");
    });

    functions["u_cameraNearClip"] = UniformFunction("float", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_cameraNearClip", activeCamera->getNearClip());
    },
    [this]() { 
        if (activeCamera)
            return vera::toString(activeCamera->getNearClip());
        return std::string("");
    });

    functions["u_cameraFarClip"] = UniformFunction("float", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_cameraFarClip", activeCamera->getFarClip());
    },
    [this]() { 
        if (activeCamera)
            return vera::toString(activeCamera->getFarClip()); 
        return std::string("");
    });

    functions["u_cameraEv100"] = UniformFunction("float", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_cameraEv100", activeCamera->getEv100());
    },
    [this]() {
        if (activeCamera)
            return vera::toString(activeCamera->getEv100());
        return std::string("");
    });

    functions["u_cameraExposure"] = UniformFunction("float", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_cameraExposure", float(activeCamera->getExposure()));
    },
    [this]() { 
        if (activeCamera)
            return vera::toString(activeCamera->getExposure());
        return std::string("");
    });

    functions["u_cameraAperture"] = UniformFunction("float", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_cameraAperture", activeCamera->getAperture());
    },
    [this]() { 
        if (activeCamera)
            return vera::toString(activeCamera->getAperture());
        return std::string("");
    });

    functions["u_cameraShutterSpeed"] = UniformFunction("float", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_cameraShutterSpeed", activeCamera->getShutterSpeed());
    },
    [this]() { 
        if (activeCamera)
            return vera::toString(activeCamera->getShutterSpeed());
        return std::string("");
    });

    functions["u_cameraSensitivity"] = UniformFunction("float", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_cameraSensitivity", activeCamera->getSensitivity());
    },
    [this]() { 
        if (activeCamera)
            return vera::toString(activeCamera->getSensitivity());
        return std::string("");
    });

    functions["u_cameraChange"] = UniformFunction("bool", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_cameraChange", activeCamera->bChange);
    },
    [this]() { 
        if (activeCamera)
            return vera::toString(activeCamera->bChange);
        return std::string("");
    });
    
    functions["u_normalMatrix"] = UniformFunction("mat3", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_normalMatrix", activeCamera->getNormalMatrix());
    });

    // CAMERA MATRIX UNIFORMS
    functions["u_viewMatrix"] = UniformFunction("mat4", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_viewMatrix", activeCamera->getViewMatrix());
    });

    functions["u_inverseViewMatrix"] = UniformFunction("mat4", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_inverseViewMatrix", activeCamera->getInverseViewMatrix());
    });

    functions["u_projectionMatrix"] = UniformFunction("mat4", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_projectionMatrix", activeCamera->getProjectionMatrix());
    });

    functions["u_inverseProjectionMatrix"] = UniformFunction("mat4", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_inverseProjectionMatrix", activeCamera->getInverseProjectionMatrix());
    });
}

Uniforms::~Uniforms(){
    clearUniforms();
}

void Uniforms::clear() {
    clearUniforms();
    vera::Scene::clear();
}

bool Uniforms::feedTo(vera::Shader *_shader, bool _lights, bool _buffers ) {
    bool update = false;

    // Pass native uniforms functions (u_time, u_data, etc...)
    for (UniformFunctionsMap::iterator it = functions.begin(); it != functions.end(); ++it) {
        if (!_lights && ( it->first == "u_scene" || it->first == "u_sceneDepth" || it->first == "u_sceneNormal" || it->first == "u_scenePosition") )
            continue;

        if (it->second.present)
            if (it->second.assign)
                it->second.assign( *_shader );
    }

    // Pass user defined uniforms (only if the shader code or scene had changed)
    // if (m_changed) 
    {
        for (UniformDataMap::iterator it = data.begin(); it != data.end(); ++it) {
            _shader->setUniform(it->first, it->second.value.data(), it->second.size);
            if (it->second.change) {
                update += true;
            }
        }
    }

    // Pass sequence uniforms (the change every frame)
    for (UniformSequenceMap::iterator it = sequences.begin(); it != sequences.end(); ++it) {
        if (it->second.size() > 0) {
            size_t frame = m_frame % it->second.size();
            _shader->setUniform(it->first, it->second[frame].value.data(), it->second[frame].size);
            update += true;
        }
    }

    // Pass Textures Uniforms
    for (vera::TexturesMap::iterator it = textures.begin(); it != textures.end(); ++it) {
        _shader->setUniformTexture(it->first, it->second, _shader->textureIndex++ );
        _shader->setUniform(it->first+"Resolution", float(it->second->getWidth()), float(it->second->getHeight()));
    }

    for (vera::TextureStreamsMap::iterator it = streams.begin(); it != streams.end(); ++it) {
        for (size_t i = 0; i < it->second->getPrevTexturesTotal(); i++)
            _shader->setUniformTexture(it->first+"Prev["+vera::toString(i)+"]", it->second->getPrevTextureId(i), _shader->textureIndex++);

        _shader->setUniform(it->first+"Time", float(it->second->getTime()));
        _shader->setUniform(it->first+"Fps", float(it->second->getFps()));
        _shader->setUniform(it->first+"Duration", float(it->second->getDuration()));
        _shader->setUniform(it->first+"CurrentFrame", float(it->second->getCurrentFrame()));
        _shader->setUniform(it->first+"TotalFrames", float(it->second->getTotalFrames()));
    }

    // Pass Buffers Texture
    if (_buffers) {
        for (size_t i = 0; i < buffers.size(); i++)
            _shader->setUniformTexture("u_buffer" + vera::toString(i), buffers[i], _shader->textureIndex++ );

        for (size_t i = 0; i < doubleBuffers.size(); i++)
            _shader->setUniformTexture("u_doubleBuffer" + vera::toString(i), doubleBuffers[i]->src, _shader->textureIndex++ );
    
        for (size_t i = 0; i < floods.size(); i++)
            _shader->setUniformTexture("u_flood" + vera::toString(i), floods[i].dst, _shader->textureIndex++ );
    }

    // Pass Convolution Piramids resultant Texture
    for (size_t i = 0; i < pyramids.size(); i++)
        _shader->setUniformTexture("u_pyramid" + vera::toString(i), pyramids[i].getResult(), _shader->textureIndex++ );

    
    if (_lights) {
        // Pass Light Uniforms
        if (lights.size() == 1) {
            vera::LightsMap::iterator it = lights.begin();
            _shader->setUniform("u_lightColor", it->second->color);
            _shader->setUniform("u_lightIntensity", it->second->intensity);
            // if (it->second->getLightType() != vera::LIGHT_DIRECTIONAL)
            _shader->setUniform("u_light", it->second->getPosition());
            if (it->second->getLightType() == vera::LIGHT_DIRECTIONAL || it->second->getLightType() == vera::LIGHT_SPOT)
                _shader->setUniform("u_lightDirection", it->second->direction);
            if (it->second->falloff > 0)
                _shader->setUniform("u_lightFalloff", it->second->falloff);
            _shader->setUniform("u_lightMatrix", it->second->getBiasMVPMatrix() );
            _shader->setUniformDepthTexture("u_lightShadowMap", it->second->getShadowMap(), _shader->textureIndex++ );
        }
        else {
            // TODO:
            //      - Lights should be pass as structs?? 

            for (vera::LightsMap::iterator it = lights.begin(); it != lights.end(); ++it) {
                std::string name = "u_" + it->first;

                _shader->setUniform(name + "Color", it->second->color);
                _shader->setUniform(name + "Intensity", it->second->intensity);
                // if (it->second->getLightType() != vera::LIGHT_DIRECTIONAL)
                _shader->setUniform(name, it->second->getPosition());
                if (it->second->getLightType() == vera::LIGHT_DIRECTIONAL || it->second->getLightType() == vera::LIGHT_SPOT)
                    _shader->setUniform(name + "Direction", it->second->direction);
                if (it->second->falloff > 0)
                    _shader->setUniform(name +"Falloff", it->second->falloff);

                _shader->setUniform(name + "Matrix", it->second->getBiasMVPMatrix() );
                _shader->setUniformDepthTexture(name + "ShadowMap", it->second->getShadowMap(), _shader->textureIndex++ );
            }
        }
        
        if (activeCubemap) {
            _shader->setUniformTextureCube("u_cubeMap", (vera::TextureCube*)activeCubemap);
            _shader->setUniform("u_SH", activeCubemap->SH, 9);
        }
    }
    return update;
}

void Uniforms::flagChange() {
    Scene::flagChange();

    // Flag all user uniforms as changed
    for (UniformDataMap::iterator it = data.begin(); it != data.end(); ++it)
        it->second.change = true;
}

void Uniforms::resetChange() {
    Scene::resetChange();
    
    // Flag all user uniforms as NOT changed
    for (UniformDataMap::iterator it = data.begin(); it != data.end(); ++it)
        m_changed += it->second.check();
}

bool Uniforms::haveChange() {             
    if (functions["u_time"].present || 
        functions["u_date"].present ||
        functions["u_delta"].present ||
        functions["u_mouse"].present)
        return true;

    return Scene::haveChange();
}

void Uniforms::checkUniforms( const std::string &_vert_src, const std::string &_frag_src ) {
    // Check active native uniforms
    for (UniformFunctionsMap::iterator it = functions.begin(); it != functions.end(); ++it) {
        std::string name = it->first + ";";
        std::string arrayName = it->first + "[";
        bool present = ( findId(_vert_src, name.c_str()) || findId(_frag_src, name.c_str()) || findId(_frag_src, arrayName.c_str()) );
        if ( it->second.present != present ) {
            it->second.present = present;
            m_changed = true;
        } 
    }
}

void Uniforms::set(const std::string& _name, float _value) {
    UniformValue value;
    value[0] = _value;
    data[_name].set(value, 1, false);
    m_changed = true;
}

void Uniforms::set(const std::string& _name, float _x, float _y) {
    UniformValue value;
    value[0] = _x;
    value[1] = _y;
    data[_name].set(value, 2, false);
    m_changed = true;
}

void Uniforms::set(const std::string& _name, float _x, float _y, float _z) {
    UniformValue value;
    value[0] = _x;
    value[1] = _y;
    value[2] = _z;
    data[_name].set(value, 3, false);
    m_changed = true;
}

void Uniforms::set(const std::string& _name, float _x, float _y, float _z, float _w) {
    UniformValue value;
    value[0] = _x;
    value[1] = _y;
    value[2] = _z;
    value[3] = _w;
    data[_name].set(value, 4, false);
    m_changed = true;
}

void Uniforms::set(const std::string& _name, const std::vector<float>& _data, bool _queue) {
    UniformValue value;
    int N = std::min((int)_data.size(), 16);
    // memcpy(&value, _data.data(), N * sizeof(float) );
    for (size_t i = 0; i < N; i++)
        value[i] = _data[i];
    data[_name].set(value, N, false, _queue);
    m_changed = true;
}

bool Uniforms::parseLine( const std::string &_line ) {
    std::vector<std::string> values = vera::split(_line,',');
    if (values.size() > 1) {
        data[ values[0] ].parse(values, 1);
        m_changed = true;
        return true;
    }
    return false;
}

bool Uniforms::addSequence( const std::string& _name, const std::string& _filename) {
    std::vector<UniformData> uniform_data_sequence;

    // Open file _filename and read all lines
    std::ifstream infile(_filename);
    std::string line = "";
    while (std::getline(infile, line)) {
        if (line.size() > 0) {
            std::vector<std::string> values = vera::split(line,',', true);
            if (values.size() > 0) {
                UniformData data;
                data.parse(values, 0, false);
                uniform_data_sequence.push_back(data);
            }
        }
    }

    // std::cout << "Load " << _name << " from " << _filename <<  " with " << uniform_data_sequence.size() << " values" << std::endl;
    if (uniform_data_sequence.size() == 0)
        return false;

    sequences[_name] = uniform_data_sequence;

    return true;
}

void Uniforms::update() {
    Scene::update();

    if (m_play) {
        m_frame++;
        if (m_frame >= std::numeric_limits<size_t>::max()-1)
            m_frame = 0;
    }
}

void Uniforms::setStreamsPlay() {
    Scene::setStreamsPlay();
    m_play = true;
}

void Uniforms::setStreamsFrame(size_t _frame) {
    Scene::setStreamsFrame(_frame);
    m_frame = _frame;
    m_play = false;
}

void Uniforms::setStreamsStop() {
    Scene::setStreamsStop();
    m_play = false;
}

void Uniforms::setStreamsRestart() {
    Scene::setStreamsRestart();
    m_frame = 0;
}


void Uniforms::printAvailableUniforms(bool _non_active) {
    if (_non_active) {
        // Print all Native Uniforms (they carry functions)
        for (UniformFunctionsMap::iterator it= functions.begin(); it != functions.end(); ++it) {                
            std::cout << "uniform " << it->second.type << ' ' << it->first << ";";
            if (it->second.print) 
                std::cout << " // " << it->second.print();
            std::cout << std::endl;
        }
    }
    else {
        // Print Native Uniforms (they carry functions) that are present on the shader
        for (UniformFunctionsMap::iterator it= functions.begin(); it != functions.end(); ++it) {                
            if (it->second.present) {
                std::cout<< "uniform " << it->second.type << ' ' << it->first << ";";
                if (it->second.print)
                    std::cout << " // " << it->second.print();
                std::cout << std::endl;
            }
        }
    }
}

void Uniforms::printDefinedUniforms(bool _csv){
    // Print user defined uniform data
    if (_csv) {
        for (UniformDataMap::iterator it= data.begin(); it != data.end(); ++it) {
            std::cout << it->first;
            for (int i = 0; i < it->second.size; i++) {
                std::cout << ',' << it->second.value[i];
            }
            std::cout << std::endl;
        }

        for (UniformSequenceMap::iterator it= sequences.begin(); it != sequences.end(); ++it) {
            if (it->second.size() > 0) {
                size_t frame = m_frame % it->second.size();
                std::cout << it->first << ',' << it->second[frame].print() << std::endl;
            }
        }
    }
    else {
        for (UniformDataMap::iterator it= data.begin(); it != data.end(); ++it) {
            std::cout << "uniform " << it->second.getType() << "  " << it->first << ";";
            for (int i = 0; i < it->second.size; i++)
                std::cout << ((i == 0)? " // " : "," ) << it->second.value[i];
            std::cout << std::endl;
        }

        for (UniformSequenceMap::iterator it= sequences.begin(); it != sequences.end(); ++it) {
            if (it->second.size() > 0) {
                size_t frame = m_frame % it->second.size();
                std::cout << "uniform " << it->second[frame].getType() << "  " << it->first << "; // " << it->second[frame].print() << std::endl;
            }
        }
    }    
}

void Uniforms::addDefine(const std::string& _define, const std::string& _value) {
    for (vera::ModelsMap::iterator it = models.begin(); it != models.end(); ++it)
        it->second->addDefine(_define, _value);
}

void Uniforms::delDefine(const std::string& _define) {
    for (vera::ModelsMap::iterator it = models.begin(); it != models.end(); ++it)
        it->second->delDefine(_define);
}

void Uniforms::printDefines() {
    for (vera::ModelsMap::iterator it = models.begin(); it != models.end(); ++it) {
        std::cout << ". \n" << std::endl;
        std::cout << "| " << it->second->getName() << std::endl;
        std::cout << "+------------- " << std::endl;
        it->second->printDefines();
    }
}

void Uniforms::printBuffers() {
    for (size_t i = 0; i < buffers.size(); i++)
        std::cout << "uniform sampler2D u_buffer" << i << ";" << std::endl;

    for (size_t i = 0; i < doubleBuffers.size(); i++)
        std::cout << "uniform sampler2D u_doubleBuffer" << i << ";" << std::endl;

    for (size_t i = 0; i < pyramids.size(); i++)
        std::cout << "uniform sampler2D u_pyramid" << i << ";" << std::endl;  

    if (functions["u_scene"].present)
        std::cout << "uniform sampler2D u_scene;" << std::endl;

    if (functions["u_sceneDepth"].present)
        std::cout << "uniform sampler2D u_sceneDepth;" << std::endl;

    if (functions["u_scenePosition"].present)
        std::cout << "uniform sampler2D u_scenePosition;" << std::endl;

    if (functions["u_sceneNormal"].present)
        std::cout << "uniform sampler2D u_sceneNormal;" << std::endl;
}

void Uniforms::clearBuffers() {
    buffers.clear();
    doubleBuffers.clear();
    pyramids.clear();
}

void Uniforms::clearUniforms() {
    data.clear();
    sequences.clear();

    for (UniformFunctionsMap::iterator it = functions.begin(); it != functions.end(); ++it)
        it->second.present = false;
}

bool Uniforms::addCameras( const std::string& _filename ) {
    std::string folder_path = vera::getBaseDir(_filename);

    std::fstream is( _filename.c_str(), std::ios::in);
    if (is.is_open()) {

        std::string line;
        size_t counter = 0;
        while (std::getline(is, line)) {
            // If line not commented 
            if (line[0] == '#')
                continue;

            // parse through row spliting into commas
            std::vector<std::string> params = vera::split(line, ',', true);

            size_t id = vera::toInt(params[0]);
            std::string model = params[1];

            // Fx,Fy,Ox,Oy,S,Qw,Qx,Qy,Qz,Tx,Ty,Tz,FILENAME
            float Fx = vera::toFloat(params[2]);  // focal length x
            float Fy = vera::toFloat(params[3]);  // focal length y
            float Ox = vera::toFloat(params[4]);  // principal point x
            float Oy = vera::toFloat(params[5]);  // principal point y
            // glm::mat4 projection = glm::mat4(
            //     2.0f * Fx, 0.0f, 0.0f, 0.0f,
            //     0.0f, 2.0f * Fy, 0.0f, 0.0f,
            //     2.0f * Ox - 1.0f, 2.0f * Oy - 1.0f, -1.0f, -1.0f,
            //     0.0f, 0.0f, -0.1f, 0.0f
            // );
            
            glm::mat4 projection = glm::mat4(
                -2.0f*Fx,           0.0f,           0.0f,       0.0f,
                0.0f,              -2.0f*Fy,        0.0f,       0.0f,
                2.0f*Ox-1.0f,       2.0f*Oy-1.0f,  -1.0f,      -1.0f,
                0.0f,               0.0f,          -0.1f,      0.0f
            );

            // Quaternion rotation
            float Qw = vera::toFloat(params[6]);
            float Qx = vera::toFloat(params[7]);
            float Qy = vera::toFloat(params[8]);
            float Qz = vera::toFloat(params[9]);
            glm::quat Q = glm::quat(Qx, Qy, Qz, Qw);
            glm::mat4 R = glm::mat4_cast(Q);
            // Translation
            float Tx = vera::toFloat(params[10]);
            float Ty = vera::toFloat(params[11]);
            float Tz = vera::toFloat(params[12]);
            glm::mat4 T = glm::mat4(
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f,-1.0f, 0.0f, 0.0f,
                0.0f, 0.0f,-1.0f, 0.0f,
                Tx,   Ty,   Tz,   1.0f
            );
            
            // Image filename (optional)
            std::string image_filename = (params.size() > 12) ? params[13] : "";

            vera::Camera* camera = new vera::Camera();
            camera->setTransformMatrix(R * T);
            camera->setProjection(projection);
            
            // Adding to VERA scene cameras
            vera::addCamera( vera::toString(counter), camera );
            // Adding to Uniforms cameras map
            cameras[ vera::toString(counter) ] = camera;
            
            std::string path = folder_path + image_filename;
            if (image_filename != "" && vera::urlExists(path)) {
                addTexture("_camera" + vera::toString(counter), path);
            }
            counter++;
        }

        std::cout << "// Added " << counter << " camera frames: " << std::endl;
        return true;
    }

    return false;
}
