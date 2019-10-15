
#include "shader.h"

#include "tools/text.h"
#include "glm/gtc/type_ptr.hpp"

#include <cstring>
#include <chrono>
#include <algorithm>
#include <iostream>

#include "shaders/default_error.h"

Shader::Shader():
    m_program(0), 
    m_fragmentShader(0),m_vertexShader(0) {

    // Adding default defines
    addDefine("GLSLVIEWER", 160);

    // Define PLATFORM
    #ifdef PLATFORM_OSX
    addDefine("PLATFORM_OSX");
    #elif defined(PLATFORM_LINUX)
    addDefine("PLATFORM_LINUX");
    #elif defined(PLATFORM_RPI) || defined(PLATFORM_RPI4)
    addDefine("PLATFORM_RPI");
    #endif
}

Shader::~Shader() {
    // Avoid crash when no command line arguments supplied
    if (m_program != 0)
        glDeleteProgram(m_program);
}

bool Shader::load(const std::string& _fragmentSrc, const std::string& _vertexSrc, bool _verbose) {
    std::chrono::time_point<std::chrono::steady_clock> start_time, end_time;
    start_time = std::chrono::steady_clock::now();
    m_defineChange = false;

    m_vertexShader = compileShader(_vertexSrc, GL_VERTEX_SHADER, _verbose);

    if (!m_vertexShader) {
        load(error_frag, error_vert, false);
        return false;
    }

    m_fragmentShader = compileShader(_fragmentSrc, GL_FRAGMENT_SHADER, _verbose);

    if (!m_fragmentShader) {
        load(error_frag, error_vert, false);
        return false;
    }

    m_program = glCreateProgram();

    glAttachShader(m_program, m_vertexShader);
    glAttachShader(m_program, m_fragmentShader);
    glLinkProgram(m_program);

    m_fragmentSource = _fragmentSrc;
    m_vertexSource = _vertexSrc;

    end_time = std::chrono::steady_clock::now();
    std::chrono::duration<double> load_time = end_time - start_time;

    GLint isLinked;
    glGetProgramiv(m_program, GL_LINK_STATUS, &isLinked);

    if (isLinked == GL_FALSE) {
        GLint infoLength = 0;
        glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &infoLength);
        if (infoLength > 1) {
            std::vector<GLchar> infoLog(infoLength);
            glGetProgramInfoLog(m_program, infoLength, NULL, &infoLog[0]);
            std::string error(infoLog.begin(),infoLog.end());
            // printf("Error linking shader:\n%s\n", error);
            std::cerr << "Error linking shader: " << error << std::endl;

            std::size_t start = error.find("line ")+5;
            std::size_t end = error.find_last_of(")");
            std::string lineNum = error.substr(start,end-start);
            std::cerr << (unsigned)toInt(lineNum) << ": " << getLineNumber(_fragmentSrc,(unsigned)toInt(lineNum)) << std::endl;
        }
        glDeleteProgram(m_program);
        load(error_frag, error_vert, false);
        return false;
    } 
    else {
        glDeleteShader(m_vertexShader);
        glDeleteShader(m_fragmentShader);

        if (_verbose) {
            std::cerr << "shader load time: " << load_time.count() << "s";
#ifdef GL_PROGRAM_BINARY_LENGTH
            GLint proglen = 0;
            glGetProgramiv(m_program, GL_PROGRAM_BINARY_LENGTH, &proglen);
            if (proglen > 0)
                std::cerr << " size: " << proglen;
#endif
#ifdef GL_PROGRAM_INSTRUCTIONS_ARB
            GLint icount = 0;
            glGetProgramivARB(m_program, GL_PROGRAM_INSTRUCTIONS_ARB, &icount);
            if (icount > 0)
                std::cerr << " #instructions: " << icount;
#endif
            std::cerr << std::endl;
        }
        return true;
    }
}

bool Shader::reload(bool _verbose) {
    return load(m_fragmentSource, m_vertexSource, _verbose);
}

const GLint Shader::getAttribLocation(const std::string& _attribute) const {
    return glGetAttribLocation(m_program, _attribute.c_str());
}

void Shader::use() {
    if (m_defineChange)
        reload(false);

    textureIndex = 0;

    if (!isInUse())
        glUseProgram(getProgram());
}

bool Shader::isInUse() const {
    GLint currentProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    return (getProgram() == (GLuint)currentProgram);
}

bool Shader::isLoaded() const {
    return m_program != 0;
}

GLuint Shader::compileShader(const std::string& _src, GLenum _type, bool _verbose) {
    std::string prolog = "";

    for(DefinesList_it it = m_defines.begin(); it != m_defines.end(); it++) {
        prolog += "#define " + it->first + " " + it->second + '\n';
    }

    // if (_verbose) {
    //     if (_type == GL_VERTEX_SHADER) {
    //         std::cout << "// ---------- Vertex Shader" << std::endl;
    //     }
    //     else {
    //         std::cout << "// ---------- Fragment Shader" << std::endl;
    //     }
    //     std::cout << prolog << std::endl;
    //     std::cout << _src << std::endl;
    // }

    prolog += "#line 0\n";

    const GLchar* sources[2] = {
        (const GLchar*) prolog.c_str(),
        (const GLchar*) _src.c_str()
    };

    GLuint shader = glCreateShader(_type);
    glShaderSource(shader, 2, sources, NULL);
    glCompileShader(shader);

    GLint isCompiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);

    GLint infoLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLength);
    
#if defined(PLATFORM_RPI) || defined(PLATFORM_RPI4) 
    if (infoLength > 1 && !isCompiled) {
#else
    if (infoLength > 1) {
#endif
        std::vector<GLchar> infoLog(infoLength);
        glGetShaderInfoLog(shader, infoLength, NULL, &infoLog[0]);
        std::cerr << (isCompiled ? "Warnings" : "Errors");
        std::cerr << " while compiling ";
        if (_type == GL_FRAGMENT_SHADER) {
            std::cerr << "fragment ";
        }
        else {
            std::cerr << "vertex ";
        }
        std::cerr << "shader:\n" << &infoLog[0] << std::endl;
    }

    if (isCompiled == GL_FALSE) {
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

void Shader::detach(GLenum _type) {
    bool vert = (GL_VERTEX_SHADER & _type) == GL_VERTEX_SHADER;
    bool frag = (GL_FRAGMENT_SHADER & _type) == GL_FRAGMENT_SHADER;

    if (vert) {
        glDeleteShader(m_vertexShader);
        glDetachShader(m_vertexShader, GL_VERTEX_SHADER);
    }

    if (frag) {
        glDeleteShader(m_fragmentShader);
        glDetachShader(m_fragmentShader, GL_FRAGMENT_SHADER);
    }
}

GLint Shader::getUniformLocation(const std::string& _uniformName) const {
    GLint loc = glGetUniformLocation(m_program, _uniformName.c_str());
    if (loc == -1){
        // std::cerr << "Uniform " << _uniformName << " not found" << std::endl;
    }
    return loc;
}

void Shader::setUniform(const std::string& _name, int _x) {
    if (isInUse()) {
        glUniform1i(getUniformLocation(_name), _x);
    }
}

void Shader::setUniform(const std::string& _name, const float *_array, unsigned int _size) {
    GLint loc = getUniformLocation(_name);
    if (isInUse()) {
        if (_size == 1) {
            glUniform1f(loc, _array[0]);
        }
        else if (_size == 2) {
            glUniform2f(loc, _array[0], _array[1]);
        }
        else if (_size == 3) {
            glUniform3f(loc, _array[0], _array[1], _array[2]);
        }
        else if (_size == 4) {
            glUniform4f(loc, _array[0], _array[1], _array[2], _array[2]);
        }
        else {
            std::cerr << "Passing matrix uniform as array, not supported yet" << std::endl;
        }
    }
}

void Shader::setUniform(const std::string& _name, const glm::vec2 *_array, unsigned int _size) {
    if (isInUse()) {
        glUniform2fv(getUniformLocation(_name), _size, glm::value_ptr(_array[0]));
    }
}

void Shader::setUniform(const std::string& _name, const glm::vec3 *_array, unsigned int _size) {
    if (isInUse()) {
        glUniform3fv(getUniformLocation(_name), _size, glm::value_ptr(_array[0]));
    }
}

void Shader::setUniform(const std::string& _name, const glm::vec4 *_array, unsigned int _size) {
    if (isInUse()) {
        glUniform4fv(getUniformLocation(_name), _size, glm::value_ptr(_array[0]));
    }
}

void Shader::setUniform(const std::string& _name, float _x) {
    if (isInUse()) {
        glUniform1f(getUniformLocation(_name), _x);
        // std::cout << "Uniform " << _name << ": float(" << _x << ")" << std::endl;
    }
}

void Shader::setUniform(const std::string& _name, float _x, float _y) {
    if (isInUse()) {
        glUniform2f(getUniformLocation(_name), _x, _y);
        // std::cout << "Uniform " << _name << ": vec2(" << _x << "," << _y << ")" << std::endl;
    }
}

void Shader::setUniform(const std::string& _name, float _x, float _y, float _z) {
    if (isInUse()) {
        glUniform3f(getUniformLocation(_name), _x, _y, _z);
        // std::cout << "Uniform " << _name << ": vec3(" << _x << "," << _y << "," << _z <<")" << std::endl;
    }
}

void Shader::setUniform(const std::string& _name, float _x, float _y, float _z, float _w) {
    if (isInUse()) {
        glUniform4f(getUniformLocation(_name), _x, _y, _z, _w);
        // std::cout << "Uniform " << _name << ": vec3(" << _x << "," << _y << "," << _z <<")" << std::endl;
    }
}

void Shader::setUniformTextureCube(const std::string& _name, const TextureCube* _tex, unsigned int _texLoc) {
    if (isInUse()) {
        glActiveTexture(GL_TEXTURE0 + _texLoc);
        glBindTexture(GL_TEXTURE_CUBE_MAP, _tex->getId());
        glUniform1i(getUniformLocation(_name), _texLoc);
    }
}

void Shader::setUniformTexture(const std::string& _name, const Texture* _tex, unsigned int _texLoc) {
    if (isInUse()) {
        glActiveTexture(GL_TEXTURE0 + _texLoc);
        glBindTexture(GL_TEXTURE_2D, _tex->getId());
        glUniform1i(getUniformLocation(_name), _texLoc);
    }
}

void Shader::setUniformTexture(const std::string& _name, const Fbo* _fbo, unsigned int _texLoc) {
    if (isInUse()) {
        glActiveTexture(GL_TEXTURE0 + _texLoc);
        glBindTexture(GL_TEXTURE_2D, _fbo->getTextureId());
        glUniform1i(getUniformLocation(_name), _texLoc);
    }
}

void Shader::setUniformDepthTexture(const std::string& _name, const Fbo* _fbo, unsigned int _texLoc) {
    if (isInUse()) {
        glActiveTexture(GL_TEXTURE0 + _texLoc);
        glBindTexture(GL_TEXTURE_2D, _fbo->getDepthTextureId());
        glUniform1i(getUniformLocation(_name), _texLoc);
    }
}

void Shader::setUniformTexture(const std::string& _name, const Texture* _tex) {
    setUniformTexture(_name, _tex, textureIndex++);
}

void  Shader::setUniformTexture(const std::string& _name, const Fbo* _fbo) {
    setUniformTexture(_name, _fbo, textureIndex++);
}

void  Shader::setUniformDepthTexture(const std::string& _name, const Fbo* _fbo) {
    setUniformDepthTexture(_name, _fbo, textureIndex++);
}

void  Shader::setUniformTextureCube(const std::string& _name, const TextureCube* _tex) {
    setUniformTextureCube(_name, _tex, textureIndex++);
}

void Shader::setUniform(const std::string& _name, const glm::mat2& _value, bool _transpose) {
    if (isInUse()) {
        glUniformMatrix2fv(getUniformLocation(_name), 1, _transpose, &_value[0][0]);
    }
}

void Shader::setUniform(const std::string& _name, const glm::mat3& _value, bool _transpose) {
    if (isInUse()) {
        glUniformMatrix3fv(getUniformLocation(_name), 1, _transpose, &_value[0][0]);
    }
}

void Shader::setUniform(const std::string& _name, const glm::mat4& _value, bool _transpose) {
    if (isInUse()) {
        glUniformMatrix4fv(getUniformLocation(_name), 1, _transpose, &_value[0][0]);
    }
}
