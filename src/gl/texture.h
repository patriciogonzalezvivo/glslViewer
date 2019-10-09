#pragma once

#include <string>

#include "gl.h"

class Texture {
public:
    Texture();
    virtual ~Texture();

    virtual bool    load(const std::string& _filepath, bool _vFlip);
    virtual bool    loadBump(const std::string& _filepath, bool _vFlip);
    virtual bool    load(int _width, int _height, int _component, int _bits, const void* _data);

    virtual void    clear();

    virtual const GLuint    getId() const { return m_id; };
    virtual std::string     getFilePath() const { return m_path; };
    virtual int             getWidth() const { return m_width; };
    virtual int             getHeight() const { return m_height; };

    /* Bind/Unbind the texture to GPU */
    virtual void    bind();
    virtual void    unbind();

protected:
    // virtual void glHandleError();

    std::string     m_path;

    int             m_width;
    int             m_height;

    GLuint          m_id;
};
