#pragma once

#ifdef SUPPORT_FOR_LIBAV

#include "textureStream.h"
#include <vector>

class TextureAudio : public TextureStream {
public:
    TextureAudio();
    virtual ~TextureAudio();

    virtual bool    load(const std::string &_path, bool _vFlip, TextureFilter _filter = LINEAR, TextureWrap _wrap = REPEAT);
    virtual bool    update();
    virtual void    clear();
private:
    static const int m_buf_len = 1024;
    std::vector<uint8_t> m_buffer_wr, m_buffer_re, m_texture;
    float* m_dft_buffer = nullptr;
};

#endif
