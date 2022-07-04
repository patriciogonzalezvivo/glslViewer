#include "uniforms.h"

#include <regex>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "tools/text.h"
#include "ada/string.h"

std::string UniformData::getType() {
    if (size == 1) return (bInt ? "int" : "float");
    else return (bInt ? "ivec" : "vec") + ada::toString(size); 
}

void UniformData::set(const UniformValue &_value, size_t _size, bool _int ) {
    bInt = _int;
    size = _size;

    if (change)
        queue.push( _value );
    else
        value = _value;
    
    change = true;
}

void UniformData::parse(const std::vector<std::string>& _command, size_t _start) {;
    UniformValue candidate;
    for (size_t i = _start; i < _command.size() && i < 5; i++)
        candidate[i-_start] = ada::toFloat(_command[i]);

    set(candidate, _command.size() - _start, true);
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

UniformFunction::UniformFunction(const std::string &_type, std::function<void(ada::Shader&)> _assign) {
    type = _type;
    assign = _assign;
}

UniformFunction::UniformFunction(const std::string &_type, std::function<void(ada::Shader&)> _assign, std::function<std::string()> _print) {
    type = _type;
    assign = _assign;
    print = _print;
}


Uniforms::Uniforms() {
    // IBL
    //
    functions["u_iblLuminance"] = UniformFunction("float", [this](ada::Shader& _shader) {
        if (m_activeCamera)
            _shader.setUniform("u_iblLuminance", 30000.0f * m_activeCamera->getExposure());
    },
    [this]() { 
        if (m_activeCamera)
            return ada::toString(30000.0f * m_activeCamera->getExposure());
        return std::string("");
    });
    
    // CAMERA UNIFORMS
    //
    functions["u_camera"] = UniformFunction("vec3", [this](ada::Shader& _shader) {
        if (m_activeCamera)
            _shader.setUniform("u_camera", - (m_activeCamera->getPosition()) );
    },
    [this]() {
        if (m_activeCamera)
            return ada::toString(-m_activeCamera->getPosition(), ',');
        return std::string("");
    });

    functions["u_cameraDistance"] = UniformFunction("float", [this](ada::Shader& _shader) {
        if (m_activeCamera)
            _shader.setUniform("u_cameraDistance", m_activeCamera->getDistance());
    },
    [this]() { 
        if (m_activeCamera)
            return ada::toString(m_activeCamera->getDistance());
        return std::string("");
    });

    functions["u_cameraNearClip"] = UniformFunction("float", [this](ada::Shader& _shader) {
        if (m_activeCamera)
            _shader.setUniform("u_cameraNearClip", m_activeCamera->getNearClip());
    },
    [this]() { 
        if (m_activeCamera)
            return ada::toString(m_activeCamera->getNearClip());
        return std::string("");
    });

    functions["u_cameraFarClip"] = UniformFunction("float", [this](ada::Shader& _shader) {
        if (m_activeCamera)
            _shader.setUniform("u_cameraFarClip", m_activeCamera->getFarClip());
    },
    [this]() { 
        if (m_activeCamera)
            return ada::toString(m_activeCamera->getFarClip()); 
        return std::string("");
    });

    functions["u_cameraEv100"] = UniformFunction("float", [this](ada::Shader& _shader) {
        if (m_activeCamera)
            _shader.setUniform("u_cameraEv100", m_activeCamera->getEv100());
    },
    [this]() {
        if (m_activeCamera)
            return ada::toString(m_activeCamera->getEv100());
        return std::string("");
    });

    functions["u_cameraExposure"] = UniformFunction("float", [this](ada::Shader& _shader) {
        if (m_activeCamera)
            _shader.setUniform("u_cameraExposure", m_activeCamera->getExposure());
    },
    [this]() { 
        if (m_activeCamera)
            return ada::toString(m_activeCamera->getExposure());
        return std::string("");
    });

    functions["u_cameraAperture"] = UniformFunction("float", [this](ada::Shader& _shader) {
        if (m_activeCamera)
            _shader.setUniform("u_cameraAperture", m_activeCamera->getAperture());
    },
    [this]() { 
        if (m_activeCamera)
            return ada::toString(m_activeCamera->getAperture());
        return std::string("");
    });

    functions["u_cameraShutterSpeed"] = UniformFunction("float", [this](ada::Shader& _shader) {
        if (m_activeCamera)
            _shader.setUniform("u_cameraShutterSpeed", m_activeCamera->getShutterSpeed());
    },
    [this]() { 
        if (m_activeCamera)
            return ada::toString(m_activeCamera->getShutterSpeed());
        return std::string("");
    });

    functions["u_cameraSensitivity"] = UniformFunction("float", [this](ada::Shader& _shader) {
        if (m_activeCamera)
            _shader.setUniform("u_cameraSensitivity", m_activeCamera->getSensitivity());
    },
    [this]() { 
        if (m_activeCamera)
            return ada::toString(m_activeCamera->getSensitivity());
        return std::string("");
    });

    functions["u_cameraChange"] = UniformFunction("bool", [this](ada::Shader& _shader) {
        if (m_activeCamera)
            _shader.setUniform("u_cameraChange", m_activeCamera->bChange);
    },
    [this]() { 
        if (m_activeCamera)
            return ada::toString(m_activeCamera->getSensitivity());
        return std::string("");
    });
    
    functions["u_normalMatrix"] = UniformFunction("mat3", [this](ada::Shader& _shader) {
        if (m_activeCamera)
            _shader.setUniform("u_normalMatrix", m_activeCamera->getNormalMatrix());
    });

    // CAMERA MATRIX UNIFORMS
    functions["u_viewMatrix"] = UniformFunction("mat4", [this](ada::Shader& _shader) {
        if (m_activeCamera)
            _shader.setUniform("u_viewMatrix", m_activeCamera->getViewMatrix());
    });

    functions["u_projectionMatrix"] = UniformFunction("mat4", [this](ada::Shader& _shader) {
        if (m_activeCamera)
            _shader.setUniform("u_projectionMatrix", m_activeCamera->getProjectionMatrix());
    });
}

Uniforms::~Uniforms(){
    clearUniforms();
}

void Uniforms::clear() {
    clearUniforms();
    ada::Scene::clear();
}


bool Uniforms::feedTo(ada::Shader &_shader, bool _lights, bool _buffers ) {
    return feedTo(&_shader, _lights, _buffers);
}

bool Uniforms::feedTo(ada::Shader *_shader, bool _lights, bool _buffers ) {
    bool update = false;

    // Pass Native uniforms 
    for (UniformFunctionsMap::iterator it = functions.begin(); it != functions.end(); ++it) {
        if (!_lights && ( it->first == "u_scene" || "u_sceneDepth") )
            continue;

        if (it->second.present)
            if (it->second.assign)
                it->second.assign( *_shader );
    }

    // Pass User defined uniforms
    if (m_change) {
        for (UniformDataMap::iterator it = data.begin(); it != data.end(); ++it) {
            if (it->second.change) {
                _shader->setUniform(it->first, it->second.value.data(), it->second.size);
                update += true;
            }
        }
    }

    ada::Scene::feedTo(_shader, _lights, _buffers);
    return update;
}

void Uniforms::flagChange() {
    // Flag all user uniforms as changed
    for (UniformDataMap::iterator it = data.begin(); it != data.end(); ++it)
        it->second.change = true;
    
    ada::Scene::flagChange();
}

void Uniforms::unflagChange() {
    if (m_change) {
        m_change = false;

        // Flag all user uniforms as NOT changed
        for (UniformDataMap::iterator it = data.begin(); it != data.end(); ++it)
            m_change += it->second.check();
    }

    ada::Scene::unflagChange();
}

bool Uniforms::haveChange() { 
    if (functions["u_time"].present || 
        functions["u_date"].present ||
        functions["u_delta"].present ||
        functions["u_mouse"].present)
        return true;

    return ada::Scene::haveChange();
}


void Uniforms::set(const std::string& _name, float _value) {
    UniformValue value;
    value[0] = _value;
    data[_name].set(value, 1, false);
    m_change = true;
}

void Uniforms::set(const std::string& _name, float _x, float _y) {
    UniformValue value;
    value[0] = _x;
    value[1] = _y;
    data[_name].set(value, 2, false);
    m_change = true;
}

void Uniforms::set(const std::string& _name, float _x, float _y, float _z) {
    UniformValue value;
    value[0] = _x;
    value[1] = _y;
    value[2] = _z;
    data[_name].set(value, 3, false);
    m_change = true;
}

void Uniforms::set(const std::string& _name, float _x, float _y, float _z, float _w) {
    UniformValue value;
    value[0] = _x;
    value[1] = _y;
    value[2] = _z;
    value[3] = _w;
    data[_name].set(value, 4, false);
    m_change = true;
}

bool Uniforms::parseLine( const std::string &_line ) {
    std::vector<std::string> values = ada::split(_line,',');
    if (values.size() > 1) {
        data[ values[0] ].parse(values, 1);
        m_change = true;
        return true;
    }
    return false;
}

void Uniforms::checkUniforms( const std::string &_vert_src, const std::string &_frag_src ) {
    // Check active native uniforms
    for (UniformFunctionsMap::iterator it = functions.begin(); it != functions.end(); ++it) {
        std::string name = it->first + ";";
        bool present = ( findId(_vert_src, name.c_str()) || findId(_frag_src, name.c_str()) );
        if ( it->second.present != present) {
            it->second.present = present;
            m_change = true;
        } 
    }
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
    }
    else {
        for (UniformDataMap::iterator it= data.begin(); it != data.end(); ++it) {
            std::cout << "uniform " << it->second.getType() << "  " << it->first << ";";
            for (int i = 0; i < it->second.size; i++)
                std::cout << ((i == 0)? " // " : "," ) << it->second.value[i];
            
            std::cout << std::endl;
        }
    }    
}

void Uniforms::printBuffers() {
    ada::Scene::printBuffers();

    if (functions["u_scene"].present)
        std::cout << "uniform sampler2D u_scene;" << std::endl;

    if (functions["u_sceneDepth"].present)
        std::cout << "uniform sampler2D u_sceneDepth;" << std::endl;
}

void Uniforms::clearUniforms() {
    data.clear();

    for (UniformFunctionsMap::iterator it = functions.begin(); it != functions.end(); ++it)
        it->second.present = false;
}

bool Uniforms::addCameraPath( const std::string& _name ) {
    std::fstream is( _name.c_str(), std::ios::in);
    if (is.is_open()) {

        glm::vec3 position;
        cameraPath.clear();

        std::string line;
        // glm::mat4 rot = glm::mat4(
        //         1.0f,   0.0f,   0.0f,   0.0f,
        //         0.0f,  1.0f,    0.0f,   0.0f,
        //         0.0f,   0.0f,   -1.0f,   0.0f,
        //         0.0f,   0.0f,   0.0f,   1.0f
        //     );

        while (std::getline(is, line)) {
            // If line not commented 
            if (line[0] == '#')
                continue;

            // parse through row spliting into commas
            std::vector<std::string> params = ada::split(line, ',', true);

            CameraData frame;
            float fL = ada::toFloat(params[0]);
            float cx = ada::toFloat(params[1]);
            float cy = ada::toFloat(params[2]);

            float near = 0.0f;
            float far = 1000.0;

            if (m_activeCamera) {
                near = m_activeCamera->getNearClip();
                far = m_activeCamera->getFarClip();
            }

            float delta = far-near;
            float w = cx*2.0f;
            float h = cy*2.0f;

            // glm::mat4 projection = glm::ortho(0.0f, w, h, 0.0f, near, far);
            // glm::mat4 ndc = glm::mat4(
            //     fL,     0.0f,   0.0f,   0.0f,
            //     0.0f,   fL,     0.0f,   0.0f, 
            //     cx,     cy,     1.0f,   0.0f,
            //     0.0f,   0.0f,   0.0f,   1.0f  
            // );
            // frame.projection = projection * ndc;

            frame.projection = glm::mat4(
                2.0f*fL/w,          0.0f,               0.0f,                       0.0f,
                0.0f,               -2.0f*fL/h,         0.0f,                       0.0f,
                (w - 2.0f*cx)/w,    (h-2.0f*cy)/h,      (-far-near)/delta,         -1.0f,
                0.0f,               0.0f,               -2.0f*far*near/delta,       0.0f
            );
            
            // frame.projection = glm::mat4(
            //     fL/cx,      0.0f,   0.0f,                   0.0f,
            //     0.0f,       fL/cy,  0.0f,                   0.0f,
            //     0.0f,       0.0f,   -(far+near)/delta,      -1.0,
            //     0.0f,       0.0f,   -2.0*far*near/delta,    0.0f
            // );

            frame.transform = glm::mat4(
                glm::vec4( ada::toFloat(params[3]), ada::toFloat(params[ 4]), ada::toFloat(params[ 5]), 0.0),
                glm::vec4( ada::toFloat(params[6]), -ada::toFloat(params[ 7]), ada::toFloat(params[ 8]), 0.0),
                glm::vec4( ada::toFloat(params[9]), ada::toFloat(params[10]), ada::toFloat(params[11]), 0.0),
                glm::vec4( ada::toFloat(params[12]), ada::toFloat(params[13]), -ada::toFloat(params[14]), 1.0)
            );

            // position += frame.translation;
            cameraPath.push_back(frame);
        }

        std::cout << "// Added " << cameraPath.size() << " camera frames" << std::endl;

        position = position / float(cameraPath.size());
        return true;
    }

    return false;
}
