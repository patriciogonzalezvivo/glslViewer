#pragma once

#include <string>
#include <vector>

#include "gl.h"
#include "glm/glm.hpp"

#include "texture.h"
#include "fbo.h"

class Shader {

public:
    Shader();
    virtual ~Shader();

    const   GLuint  getProgram() const { return m_program; };
    const   GLuint  getFragmentShader() const { return m_fragmentShader; };
    const   GLuint  getVertexShader() const { return m_vertexShader; };

    const   bool    needBackbuffer() const { return m_backbuffer; };
    const   bool    needTime() const { return m_time; };
    const   bool    needDelta() const { return m_delta; };
    const   bool    needDate() const { return m_date; };
    const   bool    needMouse() const { return m_mouse; };
    const   bool    need_iMouse() const { return m_imouse; };
    const   bool    needView2d() const { return m_view2d; };
    const   bool    needView3d() const { return m_view3d; };

    void    use() const;
    bool    isInUse() const;

    const   GLint   getAttribLocation(const std::string& _attribute) const;
    bool    load(const std::string& _fragmentSrc, const std::string& _vertexSrc, const std::vector<std::string> &_defines, bool _verbose = false);

    void    setUniform(const std::string& _name, int _x);

    void    setUniform(const std::string& _name, float _x);
    void    setUniform(const std::string& _name, float _x, float _y);
    void    setUniform(const std::string& _name, float _x, float _y, float _z);
    void    setUniform(const std::string& _name, float _x, float _y, float _z, float _w);

    void    setUniform(const std::string& _name, const float *_array, unsigned int _size);

    void    setUniform(const std::string& _name, const Texture* _tex, unsigned int _texLoc);
    void    setUniform(const std::string& _name, const Fbo* _fbo, unsigned int _texLoc);

    void    setUniform(const std::string& _name, const glm::vec2& _value) { setUniform(_name,_value.x,_value.y); }
    void    setUniform(const std::string& _name, const glm::vec3& _value) { setUniform(_name,_value.x,_value.y,_value.z); }
    void    setUniform(const std::string& _name, const glm::vec4& _value) { setUniform(_name,_value.x,_value.y,_value.z,_value.w); }

    void    setUniform(const std::string& _name, const glm::mat2& _value, bool transpose = false);
    void    setUniform(const std::string& _name, const glm::mat3& _value, bool transpose = false);
    void    setUniform(const std::string& _name, const glm::mat4& _value, bool transpose = false);

    void    detach(GLenum type);

private:

    GLuint  compileShader(const std::string& _src, const std::vector<std::string> &_defines, GLenum _type);
    GLint   getUniformLocation(const std::string& _uniformName) const;
    
    GLuint  m_program;
    GLuint  m_fragmentShader;
    GLuint  m_vertexShader;

    bool    m_backbuffer;
    bool    m_time;
    bool    m_delta;
    bool    m_date;
    bool    m_mouse;
    bool    m_imouse;
    bool    m_view2d;
    bool    m_view3d;
};
