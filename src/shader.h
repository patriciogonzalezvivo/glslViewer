#pragma once

#include <string>

#include <GLES2/gl2.h>

#include "texture.h"

class Shader {

public:
    Shader();
    virtual ~Shader();

    const   GLuint getProgram() const { return m_program; };
    const   GLuint getFragmentShader() const { return m_fragmentShader; };
    const   GLuint getVertexShader() const { return m_vertexShader; };

    void    use() const;
    bool    isInUse() const;

    const   GLint getAttribLocation(const std::string& _attribute);
    bool    load(const std::string& _fragmentSrc, const std::string& _vertexSrc);

    void    sendUniform(const std::string& _name, float _x);
    void    sendUniform(const std::string& _name, float _x, float _y);
    void    sendUniform(const std::string& _name, float _x, float _y, float _z);
    void    sendUniform(const std::string& _name, const Texture* _tex, unsigned int _texLoc);

    void    detach(GLenum type);

private:

    GLuint  compileShader(const std::string& _src, GLenum _type);
    GLint   getUniformLocation(const std::string& _uniformName) const;
    
    GLuint  m_program;
    GLuint  m_fragmentShader;
    GLuint  m_vertexShader;
};
