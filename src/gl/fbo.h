#pragma once

#include "gl.h"

class Fbo {
public:
    Fbo();
    Fbo(unsigned int _width, unsigned int _height);
    virtual ~Fbo();

    const GLuint getId() const { return m_id; };
    const GLuint getTextureId() const { return m_color_texture; };

    void resize(const unsigned int _width, const unsigned int _height);

    void bind();
    void unbind();

protected:
    unsigned int m_width;
    unsigned int m_height;

    GLuint  m_id;
    GLuint  m_old_fbo_id;

    GLuint  m_color_texture;
    GLuint  m_depth_texture;
};
