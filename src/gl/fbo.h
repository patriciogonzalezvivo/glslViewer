#pragma once

#include "gl.h"

class Fbo {
public:
    Fbo();
    virtual ~Fbo();

    const GLuint getId() const { return m_id; };
    const GLuint getTextureId() const { return m_texture; };

    void allocate(const unsigned int _width, const unsigned int _height, bool _depth = true);

    void bind();
    void unbind();

protected:
    GLuint  m_id;
    GLuint  m_old_fbo_id;

    GLuint  m_texture;
    GLuint  m_depth_buffer;

    unsigned int m_width;
    unsigned int m_height;

    bool    m_allocated;
    bool    m_binded;
};
