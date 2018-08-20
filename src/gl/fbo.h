#pragma once

#include "gl.h"

class Fbo {
public:
    Fbo();
    virtual ~Fbo();

    virtual void    allocate(const unsigned int _width, const unsigned int _height, bool _depth = false, bool _depth_texture = false);
    virtual void    bind();
    virtual void    unbind();
    
    const GLuint    getId() const { return m_id; }
    const GLuint    getTextureId() const { return m_texture; }

    const bool      haveDepthBuffer() const { return m_depth; }
    const bool      haveDepthTexture() const { return m_depth_texture != 0; }
    const GLuint    getDepthTextureId() const { return m_depth_texture; }

    virtual int		getWidth() const { return m_width; };
	virtual int		getHeight() const { return m_height; };

protected:
    GLuint  m_id;
    GLuint  m_old_fbo_id;

    GLuint  m_texture;
    GLuint  m_depth_buffer;
    GLuint  m_depth_texture;

    int     m_width;
    int     m_height;

    bool    m_allocated;
    bool    m_binded;
    bool    m_depth;
};
