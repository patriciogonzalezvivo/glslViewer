
#pragma once

#include "textureStream.h"
#include <vector>

class TextureStreamSequence : public TextureStream {
public:
    TextureStreamSequence();
    virtual ~TextureStreamSequence();

    virtual int     getTotalFrames() { return m_frames.size(); };
    virtual int     getCurrentFrame() { return m_currentFrame; };

    virtual bool    load(const std::string& _filepath, bool _vFlip);
    virtual bool    update();
    virtual void    clear();

private:

    std::vector<void*>  m_frames;
    size_t  m_currentFrame;
    size_t  m_bits;

};
