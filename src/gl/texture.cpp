#include <iostream>

#include "texture.h"

#include "../io/fs.h"
#include "../io/pixels.h"

// TEXTURE
Texture::Texture():m_path(""), m_width(0), m_height(0), m_id(0), m_vFlip(false) {
}

Texture::~Texture() {
    clear();
}

void Texture::clear() {
    if (m_id != 0)
        glDeleteTextures(1, &m_id);
    m_id = 0;
}

bool Texture::load(const std::string& _path, bool _vFlip, TextureFilter _filter, TextureWrap _wrap) {
    
    std::string ext = getExt(_path);

    // BMP non-1bpp, non-RLE
    // GIF (*comp always reports as 4-channel)
    // JPEG baseline & progressive (12 bpc/arithmetic not supported, same as stock IJG lib)
    if (ext == "bmp"    || ext == "BMP" ||
        ext == "gif"    || ext == "GIF" ||
        ext == "jpg"    || ext == "JPG" ||
        ext == "jpeg"   || ext == "JPEG" ) {

        unsigned char* pixels = loadPixels(_path, &m_width, &m_height, RGB_ALPHA, _vFlip);
        load(m_width, m_height, 4, 8, pixels, _filter, _wrap);
        freePixels(pixels);
    }

    // PNG 1/2/4/8/16-bit-per-channel
    // TGA (not sure what subset, if a subset)
    // PSD (composited view only, no extra channels, 8/16 bit-per-channel)
    else if (   ext == "png" || ext == "PNG" ||
                ext == "psd" || ext == "PSD" ||
                ext == "tga" || ext == "TGA") {
#ifdef PLATFORM_RPI
        // If we are in a Raspberry Pi don't take the risk of loading a 16bit image
        unsigned char* pixels = loadPixels(_path, &m_width, &m_height, RGB_ALPHA, _vFlip);
        load(m_width, m_height, 4, 8, pixels, _filter, _wrap);
        freePixels(pixels);
#else
        uint16_t* pixels = loadPixels16(_path, &m_width, &m_height, RGB_ALPHA, _vFlip);
        load(m_width, m_height, 4, 16, pixels, _filter, _wrap);
        freePixels(pixels);
#endif
    }

    // HDR (radiance rgbE format)
    else if (ext == "hdr" || ext == "HDR") {
        float* pixels = loadPixelsHDR(_path, &m_width, &m_height, _vFlip);
        load(m_width, m_height, 3, 32, pixels, _filter, _wrap);
        freePixels(pixels);
    }

    m_path = _path;
    m_vFlip = _vFlip;

    return true;
}

bool Texture::load(int _width, int _height, int _channels, int _bits, const void* _data, TextureFilter _filter, TextureWrap _wrap) {

    GLenum format = GL_RGBA;
    if (_channels == 4)         format = GL_RGBA;
    else if (_channels == 3)    format = GL_RGB;
#if !defined(PLATFORM_RPI)
    else if (_channels == 2)    format = GL_RG;
    else if (_channels == 1)    format = GL_RED;
#endif
    else
        std::cout << "Unrecognize GLenum format " << _channels << std::endl;

    GLenum type = GL_UNSIGNED_BYTE;
    if (_bits == 32)        type = GL_FLOAT;
    else if (_bits == 16)   type = GL_UNSIGNED_SHORT;
    else if (_bits == 8)    type = GL_UNSIGNED_BYTE;
    else 
        std::cout << "Unrecognize GLenum type for " << _bits << " bits" << std::endl;

    if (_width == m_width && _height == m_height &&
        _filter == m_filter && _wrap == m_wrap &&
        format == m_format && type == m_type)
        return update(0,0,_width,_height, _data);

    m_width = _width;
    m_height = _height;
    m_format = format;
    m_type = type;
    m_filter = _filter;
    m_wrap = _wrap;

    // Generate an OpenGL texture ID for this texturez
    glEnable(GL_TEXTURE_2D);
    if (m_id == 0)
        glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, getMinificationFilter(m_filter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, getMagnificationFilter(m_filter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, getWrap(m_wrap));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, getWrap(m_wrap));
    
#ifdef PLATFORM_RPI
    int max_size = std::max(m_width, m_height);
    if ( max_size > 1024) {
        float factor = max_size/1024.0;
        int w = m_width/factor;
        int h = m_height/factor;

        if (_bits == 32) {
            float * data = new float [w * h * _channels];
            rescalePixels((float*)_data, m_width, m_height, _channels, w, h, data);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, format, type, data);
            delete[] data;
        }
        else if (_bits == 16) {
            unsigned short * data = new unsigned short [w * h * _channels];
            rescalePixels((unsigned short *)_data, m_width, m_height, _channels, w, h, data);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, format, type, data);
            delete[] data;
        }
        else if (_bits == 8) {
            unsigned char * data = new unsigned char [w * h * _channels];
            rescalePixels((unsigned char*)_data, m_width, m_height, _channels, w, h, data);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, format, type, data);
            delete[] data;
        }
        m_width = w;
        m_height = h;
    }
    else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, format, type, _data);
#else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, format, type, _data);
#endif
    return true;
}

bool Texture::update(int _x, int _y, int _width, int _height, const void* _data) {
    // glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, _x, _y, _width, _height, m_format, m_type, _data);
    return true;
}

void Texture::bind() {
    // glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture::unbind() {
    glBindTexture(GL_TEXTURE_2D, 0);
}
