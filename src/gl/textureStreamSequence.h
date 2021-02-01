
#pragma once

#include "textureStream.h"
#include <vector>

class TextureStreamSequence : public TextureStream {
public:
    TextureStreamSequence();
    virtual ~TextureStreamSequence();

    virtual bool    load(const std::string& _filepath, bool _vFlip);
    virtual bool    update();
    virtual void    clear();

private:

    std::vector<unsigned char*>  m_frames;
    size_t  m_currentFrame;

};
