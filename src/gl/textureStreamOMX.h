#pragma once

#include "textureStream.h"

#ifdef PLATFORM_RPI

#include <thread>

class TextureStreamOMX : public TextureStream {
public:
    TextureStreamOMX();
    virtual ~TextureStreamOMX();

    virtual bool    load(const std::string& _filepath, bool _vFlip);
    virtual bool    update() { return true; }
    virtual void    clear();

protected:
    void*       m_eglImage;
    std::thread m_thread;

};
#endif