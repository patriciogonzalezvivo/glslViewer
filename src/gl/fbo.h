#pragma once

#include "gl.h"

enum FboType {
    COLOR_TEXTURE = 0, 
    COLOR_TEXTURE_DEPTH_BUFFER, 
    COLOR_DEPTH_TEXTURES, 
    DEPTH_TEXTURE
};

enum FboFilter {
    LINEAR = 0,     // Bilinear, no mipmap.
    NEAREST        // Nearest neighbour, no mipmap.
    // NEAREST_NEAREST,// Nearest neighbour, closest mipmap.
    // LINEAR_NEAREST, // Bilinear, closest mipmap.
    // NEAREST_LINEAR, // Nearest neighbour, linear blend of mipmaps.
    // LINEAR_LINEAR   // Bilinear, linear blend of mipmaps.
};

enum FboWrap {
    REPEAT = 0,     // Repeat the texture.
    CLAMP          // Clamp to the edges of the texture.
    // MIRROR
};

class Fbo {
public:
    Fbo();
    virtual ~Fbo();

    virtual void    allocate(const unsigned int _width, const unsigned int _height, FboType _type, FboFilter _filter = LINEAR, FboWrap _wrap = REPEAT);
    virtual void    bind();
    virtual void    unbind();
    
    const GLuint    getId() const { return m_fbo_id; }
    const FboType&  getType() const { return m_type; }
    const GLuint    getTextureId() const { return m_id; }

    const bool      isAllocated() const { return m_allocated; }
    const bool      haveDepthBuffer() const { return m_depth; }
    const bool      haveDepthTexture() const { return m_depth_id != 0; }
    const GLuint    getDepthTextureId() const { return m_depth_id; }

    virtual int     getWidth() const { return m_width; };
    virtual int     getHeight() const { return m_height; };

protected:
    GLuint  m_id;
    GLuint  m_fbo_id;
    GLuint  m_old_fbo_id;

    GLuint  m_depth_id;
    GLuint  m_depth_buffer;

    FboType m_type;

    int     m_width;
    int     m_height;

    bool    m_allocated;
    bool    m_binded;
    bool    m_depth;
};
