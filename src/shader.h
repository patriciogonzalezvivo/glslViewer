#pragma once

#include <string>
#include <iostream>

#include "GLES2/gl2.h"

class Shader {

public:
    Shader();
    virtual ~Shader();

    const GLuint getProgram() const { return program; };
    const GLuint getFragmentShader() const { return fragmentShader; };
    const GLuint getVertexShader() const { return vertexShader; };

    void use() const;
    bool isInUse() const;

    const GLint getAttribLocation(const std::string& attribute);
    bool build(const std::string& fragmentSrc, const std::string& vertexSrc);

    void sendUniform(const std::string& name, float x);
    void sendUniform(const std::string& name, float x, float y);
    void sendUniform(const std::string& name, float x, float y, float z);

    void detach(GLenum type);

private:

    GLuint link();
    GLuint compileShader(const std::string& src, GLenum type);
    GLint getUniformLocation(const std::string& uniformName) const;
    
    GLuint program;
    GLuint fragmentShader;
    GLuint vertexShader;
};
