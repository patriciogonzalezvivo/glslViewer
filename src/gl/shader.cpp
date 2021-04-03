
#include "shader.h"

#include "tools/text.h"
#include "glm/gtc/type_ptr.hpp"

#include <cstring>
#include <chrono>
#include <algorithm>
#include <iostream>

#include "shaders/default_error.h"

Shader::Shader():
    m_fragmentSource(error_frag),
    m_vertexSource(error_vert),
    m_program(0), 
    m_fragmentShader(0),m_vertexShader(0) {


    // Adding default defines
    addDefine("GLSLVIEWER", 160);

    // Define PLATFORM
    #ifdef PLATFORM_OSX
    addDefine("PLATFORM_OSX");
    #elif defined(PLATFORM_LINUX)
    addDefine("PLATFORM_LINUX");
    #elif defined(PLATFORM_RPI)
    addDefine("PLATFORM_RPI");
    #endif
}

Shader::~Shader() {
    // Avoid crash when no command line arguments supplied
    if (m_program != 0)
        glDeleteProgram(m_program);
}

bool Shader::load(const std::string& _fragmentSrc, const std::string& _vertexSrc, bool _verbose, bool _error_screen) {
    std::chrono::time_point<std::chrono::steady_clock> start_time, end_time;
    start_time = std::chrono::steady_clock::now();
    m_defineChange = false;

    m_vertexShader = compileShader(_vertexSrc, GL_VERTEX_SHADER, _verbose);

    if (!m_vertexShader) {
        if (_error_screen)
            load(error_frag, error_vert, false);
        else
            load(m_fragmentSource, m_vertexSource, false);

        return false;
    }

    m_fragmentShader = compileShader(_fragmentSrc, GL_FRAGMENT_SHADER, _verbose);

    if (!m_fragmentShader) {
        if (_error_screen)
            load(error_frag, error_vert, false);
        else
            load(m_fragmentSource, m_vertexSource, false);

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
    textureIndex = 0;

    if (m_defineChange)
        reload(false);

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

    //
    // detect #version directive at the beginning of the shader, move it to the prolog and remove it from the shader
    //

    std::string srcBody; // _src stripped of any #version directive at the beginning
    bool zeroBasedLineDirective; // true for GLSL core 1.10 to 1.50
    bool srcVersionFound = _src.substr(0, 8) == "#version"; // true if user provided a #version directive at the beginning of _src

    if(srcVersionFound) {

        //
        // split _src into srcVersion and srcBody
        //

        std::istringstream srcIss(_src);

        // the version line can be read without checking the result of getline(), srcVersionFound == true implies this
        std::string srcVersion;
        std::getline(srcIss,srcVersion);

        // move the #version directive to the top of the prolog
        prolog += srcVersion + '\n';

        // copy the rest of the shader into srcBody
        std::ostringstream srcOss("");
        std::string dataRead;
        while(std::getline(srcIss,dataRead)){
            srcOss << dataRead << '\n';
        }
        srcBody = srcOss.str();

        //
        // try to determine which version are we actually using
        //

        size_t glslVersionNumber = 0;
        std::istringstream versionIss(srcVersion);
        versionIss >> dataRead; // consume the "#version" string which is guaranteed to be there
        versionIss >> glslVersionNumber; // try to read the next token and convert it to a number

        //
        // determine if the glsl version number starts numbering the #line directive from 0 or from 1
        //
        // #version 100        : "es"   profile, numbering starts from 1
        // #version 110 to 150 : "core" profile, numbering starts from 0
        // #version 200        : "es"   profile, numbering starts from 1
        // #version 300 to 320 : "es"   profile, numbering starts from 1
        // #version 330+       : all           , numbering starts from 1
        //
        // Any malformed or invalid #version directives are of no interest here, the shader compiler
        // will take care of reporting this to the user later.
        //

        zeroBasedLineDirective = (glslVersionNumber >= 110 && glslVersionNumber <= 150);

    } else {
        // no #version directive found at the beginning of _src, which means...
        srcBody = _src; // ... _src contains the whole shader body and ...
        zeroBasedLineDirective = true; // ... glsl defaults to version 1.10, which starts numbering #line directives from 0.
    }

    for(DefinesList_it it = m_defines.begin(); it != m_defines.end(); it++) {
        prolog += "#define " + it->first + " " + it->second + '\n';
    }

    //
    // determine the #line offset to be used for conciliating lines in glsl error messages and the line number in the editor
    //
    // #line 0 : no version specified by user, glsl defaults to version 1.10, shader source used without modifications
    // #line 1 : user specified #version 1.10 to 1.50, which start numbering #line from 0, version line removed from _src
    // #line 2 : user specified a version #version other than 1.10 to 1.50, those start numbering #line from 1, version line removed from _src
    //

    size_t startLine = (srcVersionFound ? 1 : 0) + (zeroBasedLineDirective ? 0 : 1);
    prolog += "#line " + std::to_string(startLine) + "\n";

    // if (_verbose) {
    //     if (_type == GL_VERTEX_SHADER) {
    //         std::cout << "// ---------- Vertex Shader" << std::endl;
    //     }
    //     else {
    //         std::cout << "// ---------- Fragment Shader" << std::endl;
    //     }
    //     std::cout << prolog << std::endl;
    //     std::cout << srcBody << std::endl;
    // }

    const GLchar* sources[2] = {
        (const GLchar*) prolog.c_str(),
        (const GLchar*) srcBody.c_str()
    };

    GLuint shader = glCreateShader(_type);
    glShaderSource(shader, 2, sources, NULL);
    glCompileShader(shader);

    GLint isCompiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);

    GLint infoLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLength);
    
#ifdef PLATFORM_RPI
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

void Shader::setUniform(const std::string& _name, int _x, int _y) {
    if (isInUse()) {
        glUniform2i(getUniformLocation(_name), _x, _y);
        // std::cout << "Uniform " << _name << ": vec2i(" << _x << "," << _y << ")" << std::endl;
    }
}

void Shader::setUniform(const std::string& _name, int _x, int _y, int _z) {
    if (isInUse()) {
        glUniform3i(getUniformLocation(_name), _x, _y, _z);
        // std::cout << "Uniform " << _name << ": vec3i(" << _x << "," << _y << "," << _z <<")" << std::endl;
    }
}

void Shader::setUniform(const std::string& _name, int _x, int _y, int _z, int _w) {
    if (isInUse()) {
        glUniform4i(getUniformLocation(_name), _x, _y, _z, _w);
        // std::cout << "Uniform " << _name << ": vec4i(" << _x << "," << _y << "," << _z << << "," << _w << ")" << std::endl;
    }
}

void Shader::setUniform(const std::string& _name, const int *_array, unsigned int _size) {
    GLint loc = getUniformLocation(_name);
    if (isInUse()) {
        if (_size == 1) {
            glUniform1i(loc, _array[0]);
        }
        else if (_size == 2) {
            glUniform2i(loc, _array[0], _array[1]);
            std::cout << _name << ',' << _array[0] << ',' << _array[1] << std::endl;
        }
        else if (_size == 3) {
            glUniform3i(loc, _array[0], _array[1], _array[2]);
        }
        else if (_size == 4) {
            glUniform4i(loc, _array[0], _array[1], _array[2], _array[3]);
        }
        else {
            std::cerr << "Passing matrix uniform as array, not supported yet" << std::endl;
        }
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

void Shader::setUniformTexture(const std::string& _name, GLuint _textureId, unsigned int _texLoc) {
    if (isInUse()) {
        glActiveTexture(GL_TEXTURE0 + _texLoc);
        glBindTexture(GL_TEXTURE_2D, _textureId);
        glUniform1i(getUniformLocation(_name), _texLoc);
    }
}

void Shader::setUniformTexture(const std::string& _name, const Texture* _tex, unsigned int _texLoc) {
    setUniformTexture(_name, _tex->getTextureId(), _texLoc);
}

void Shader::setUniformTexture(const std::string& _name, const Fbo* _fbo, unsigned int _texLoc) {
    setUniformTexture(_name, _fbo->getTextureId(), _texLoc);
}

void Shader::setUniformDepthTexture(const std::string& _name, const Fbo* _fbo, unsigned int _texLoc) {
    setUniformTexture(_name, _fbo->getDepthTextureId(), _texLoc);
}

void Shader::setUniformTexture(const std::string& _name, const Texture* _tex) {
    setUniformTexture(_name, _tex->getTextureId(), textureIndex++);
}

void  Shader::setUniformTexture(const std::string& _name, const Fbo* _fbo) {
    setUniformTexture(_name, _fbo->getTextureId(), textureIndex++);
}

void  Shader::setUniformDepthTexture(const std::string& _name, const Fbo* _fbo) {
    setUniformTexture(_name, _fbo->getDepthTextureId(), textureIndex++);
}

void Shader::setUniformTextureCube(const std::string& _name, const TextureCube* _tex, unsigned int _texLoc) {
    if (isInUse()) {
        glActiveTexture(GL_TEXTURE0 + _texLoc);
        glBindTexture(GL_TEXTURE_CUBE_MAP, _tex->getTextureId());
        glUniform1i(getUniformLocation(_name), _texLoc);
    }
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
