#pragma once

#include "gl.h"

class Fbo {
public:
    Fbo();
    virtual ~Fbo();

    const GLuint getId() const { return m_id; };
    const GLuint getTextureId() const { return m_texture; };
    const GLuint getDepthTextureId() const { return m_depth_texture; };

    void allocate(const unsigned int _width, const unsigned int _height);

    void bind();
    void unbind();

protected:
    unsigned int m_width;
    unsigned int m_height;

    GLuint  m_id;
    GLuint  m_old_fbo_id;

    GLuint  m_texture;
    GLuint  m_depth_texture;

    bool    m_allocated;
    bool    m_binded;
};
