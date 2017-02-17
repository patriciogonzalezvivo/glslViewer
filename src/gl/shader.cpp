#include "shader.h"

#include <cstring>
#include "utils.h"
#include <chrono>
#include <iostream>

Shader::Shader():m_program(0),m_fragmentShader(0),m_vertexShader(0), m_backbuffer(0), m_time(false), m_delta(false), m_date(false), m_mouse(false), m_imouse(false), m_view2d(false) {

}

Shader::~Shader() {
    if (m_program != 0) {           // Avoid crash when no command line arguments supplied
        glDeleteProgram(m_program);
    }
}

std::string getLineNumber(const std::string& _source, unsigned _lineNumber){
    std::string delimiter = "\n";
    std::string::const_iterator substart = _source.begin(), subend;

    unsigned index = 1;
    while (true) {
        subend = search(substart, _source.end(), delimiter.begin(), delimiter.end());
        std::string sub(substart, subend);
        
        if (index == _lineNumber) {
            return sub;
        }
        index++;

        if (subend == _source.end()) {
            break;
        }

        substart = subend + delimiter.size();
    }

    return "NOT FOUND";
}

// Quickly determine if a shader program contains the specified identifier.
bool find_id(const std::string& program, const char* id) {
    return std::strstr(program.c_str(), id) != 0;
}

bool Shader::load(const std::string& _fragmentSrc, const std::string& _vertexSrc, bool verbose) {
    std::chrono::time_point<std::chrono::steady_clock> start_time, end_time;
    start_time = std::chrono::steady_clock::now();

    m_vertexShader = compileShader(_vertexSrc, GL_VERTEX_SHADER);

    if(!m_vertexShader) {
        glDeleteShader(m_vertexShader);
        std::cout << "Error loading vertex shader src" << std::endl;
        return false;
    }

    m_fragmentShader = compileShader(_fragmentSrc, GL_FRAGMENT_SHADER);

    if(!m_fragmentShader) {
        std::cout << "Error loading fragment shader src" << std::endl;
        glDeleteShader(m_fragmentShader);
        return false;
    } else {
        m_backbuffer = find_id(_fragmentSrc, "u_backbuffer");
        if (!m_time)
            m_time = find_id(_fragmentSrc, "u_time");
        if (!m_delta)
            m_delta = find_id(_fragmentSrc, "u_delta");
        if (!m_date)
            m_date = find_id(_fragmentSrc, "u_date");
        m_mouse = find_id(_fragmentSrc, "u_mouse");
        m_view2d = find_id(_fragmentSrc, "u_view2d");
    }

    m_program = glCreateProgram();

    glAttachShader(m_program, m_vertexShader);
    glAttachShader(m_program, m_fragmentShader);    
    glLinkProgram(m_program);

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
            std::cerr << (unsigned)getInt(lineNum) << ": " << getLineNumber(_fragmentSrc,(unsigned)getInt(lineNum)) << std::endl;
        }
        glDeleteProgram(m_program);
        return false;
    } else {
        glDeleteShader(m_vertexShader);
        glDeleteShader(m_fragmentShader);
#ifndef #ifdef PLATFORM_RPI
        if (verbose) {
            std::cerr << "shader load time: " << load_time.count() << "s";
            GLint proglen = 0;
            glGetProgramiv(m_program, GL_PROGRAM_BINARY_LENGTH, &proglen);
            if (proglen > 0)
                std::cerr << " size: " << proglen;
            GLint icount = 0;
            glGetProgramivARB(m_program, GL_PROGRAM_INSTRUCTIONS_ARB, &icount);
            if (icount > 0)
                std::cerr << " #instructions: " << icount;
            std::cerr << "\n";
        }
#endif
        return true;
    }
}

const GLint Shader::getAttribLocation(const std::string& _attribute) const {
    return glGetAttribLocation(m_program, _attribute.c_str());
}

void Shader::use() const {
    if(!isInUse()) {
        glUseProgram(getProgram());
    }
}

bool Shader::isInUse() const {
    GLint currentProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    return (getProgram() == (GLuint)currentProgram);
}

GLuint Shader::compileShader(const std::string& _src, GLenum _type) {
    std::string prolog;
    const char* epilog = "";

    // Test if this is a shadertoy.com image shader. If it is, we need to
    // define some uniforms with different names than the glslViewer standard,
    // and we need to add prolog and epilog code.
    if (_type == GL_FRAGMENT_SHADER && find_id(_src, "mainImage")) {
        epilog =
            "\n"
            "void main(void) {\n"
            "    mainImage(gl_FragColor, gl_FragCoord.st);\n"
            "}\n";
        prolog =
            "#define __GLSLVIEWER__ 1\n"
            "uniform vec2 u_resolution;\n"
            "#define iResolution vec3(u_resolution, 1.0)\n"
            "\n";
        m_time = find_id(_src, "iGlobalTime");
        if (m_time) {
            prolog +=
                "uniform float u_time;\n"
                "#define iGlobalTime u_time\n"
                "\n";
        }
        m_delta = find_id(_src, "iTimeDelta");
        if (m_delta) {
            prolog +=
                "uniform float u_delta;\n"
                "#define iTimeDelta u_delta\n"
                "\n";
        }
        m_date = find_id(_src, "iDate");
        if (m_date) {
            prolog +=
                "uniform vec4 u_date;\n"
                "#define iDate u_date\n"
                "\n";
        }
        m_imouse = find_id(_src, "iMouse");
        if (m_imouse) {
            prolog +=
                "uniform vec4 iMouse;\n"
                "\n";
        }
    }

    const GLchar* sources[3] = {
        (const GLchar*) prolog.c_str(),
        (const GLchar*) _src.c_str(),
        (const GLchar*) epilog,
    };

    GLuint shader = glCreateShader(_type);
    glShaderSource(shader, 3, sources, NULL);
    glCompileShader(shader);
    
    GLint isCompiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);

    if (isCompiled == GL_FALSE) {
        GLint infoLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLength);
        if (infoLength > 1) {
            std::vector<GLchar> infoLog(infoLength);
            glGetShaderInfoLog(shader, infoLength, NULL, &infoLog[0]);
            printf("Error compiling shader:\n%s\n", &infoLog[0]);
        }
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

void Shader::detach(GLenum _type) {
    bool vert = (GL_VERTEX_SHADER & _type) == GL_VERTEX_SHADER;
    bool frag = (GL_FRAGMENT_SHADER & _type) == GL_FRAGMENT_SHADER;
    
    if(vert) {
        glDeleteShader(m_vertexShader);
        glDetachShader(m_vertexShader, GL_VERTEX_SHADER);
    }

    if(frag) {
        glDeleteShader(m_fragmentShader);
        glDetachShader(m_fragmentShader, GL_FRAGMENT_SHADER);
    }
}

GLint Shader::getUniformLocation(const std::string& _uniformName) const {
    GLint loc = glGetUniformLocation(m_program, _uniformName.c_str());
    if(loc == -1){
        // std::cerr << "Uniform " << _uniformName << " not found" << std::endl;
    }
    return loc;
}

void Shader::setUniform(const std::string& _name, const float *_array, unsigned int _size) {
    GLint loc = getUniformLocation(_name); 
    if(isInUse()) {
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
            std::cout << "Passing matrix uniform as array, not supported yet" << std::endl;
        }
    }
}

void Shader::setUniform(const std::string& _name, float _x) {
    if(isInUse()) {
        glUniform1f(getUniformLocation(_name), _x);
        // std::cout << "Uniform " << _name << ": float(" << _x << ")" << std::endl;
    }
}

void Shader::setUniform(const std::string& _name, float _x, float _y) {
    if(isInUse()) {
        glUniform2f(getUniformLocation(_name), _x, _y);
        // std::cout << "Uniform " << _name << ": vec2(" << _x << "," << _y << ")" << std::endl;
    }
}

void Shader::setUniform(const std::string& _name, float _x, float _y, float _z) {
    if(isInUse()) {
        glUniform3f(getUniformLocation(_name), _x, _y, _z);
        // std::cout << "Uniform " << _name << ": vec3(" << _x << "," << _y << "," << _z <<")" << std::endl;
    }
}

void Shader::setUniform(const std::string& _name, float _x, float _y, float _z, float _w) {
    if(isInUse()) {
        glUniform4f(getUniformLocation(_name), _x, _y, _z, _w);
        // std::cout << "Uniform " << _name << ": vec3(" << _x << "," << _y << "," << _z <<")" << std::endl;
    }
}

void Shader::setUniform(const std::string& _name, const Texture* _tex, unsigned int _texLoc){
    if(isInUse()) {
        glActiveTexture(GL_TEXTURE0 + _texLoc);
        glBindTexture(GL_TEXTURE_2D, _tex->getId());
        glUniform1i(getUniformLocation(_name), _texLoc);
    }
}

void Shader::setUniform(const std::string& _name, const Fbo* _fbo, unsigned int _texLoc){
    if(isInUse()) {
        glActiveTexture(GL_TEXTURE0 + _texLoc);
        glBindTexture(GL_TEXTURE_2D, _fbo->getTextureId());
        glUniform1i(getUniformLocation(_name), _texLoc);
    }
}

void Shader::setUniform(const std::string& _name, const glm::mat2& _value, bool _transpose){
    if(isInUse()) {
        glUniformMatrix2fv(getUniformLocation(_name), 1, _transpose, &_value[0][0]);
    }
}

void Shader::setUniform(const std::string& _name, const glm::mat3& _value, bool _transpose){
    if(isInUse()) {
        glUniformMatrix3fv(getUniformLocation(_name), 1, _transpose, &_value[0][0]);
    }
}

void Shader::setUniform(const std::string& _name, const glm::mat4& _value, bool _transpose){
    if(isInUse()) {
        glUniformMatrix4fv(getUniformLocation(_name), 1, _transpose, &_value[0][0]);
    }
}
