#pragma once

#if defined(SUPPORT_LIBAV) && defined(SUPPORT_AUDIO)

#include "ada/gl/textureStream.h"
#include <vector>

class TextureAudio : public ada::TextureStream {
public:
    TextureAudio();
    virtual ~TextureAudio();

    virtual bool    load(const std::string &_path, bool _vFlip, ada::TextureFilter _filter = ada::LINEAR, ada::TextureWrap _wrap = ada::REPEAT);
    virtual bool    update();
    virtual void    clear();
private:
    static const int        m_buf_len = 1024;
    std::vector<uint8_t>    m_buffer_wr, m_buffer_re, m_texture;
    float*                  m_dft_buffer = nullptr;
};


#endif
