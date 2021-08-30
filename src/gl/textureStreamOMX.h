#pragma once

#include "textureStream.h"

#ifdef DRIVER_LEGACY

#include <thread>

class TextureStreamOMX: public TextureStream {
public:
    TextureStreamOMX();
    virtual ~TextureStreamOMX();

    virtual bool    load(const std::string& _filepath, bool _vFlip, TextureFilter _filter = LINEAR, TextureWrap _wrap = REPEAT);
    virtual bool    update() { return true; }
    virtual void    clear();

protected:
    static void* decode_video(const char* filename, void* _streamTexture);

    void*       m_eglImage;
    std::thread m_thread;
    bool        m_changed;

};
#endif