#include <iostream>

#include "texture.h"

#include "glm/glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/normal.hpp>

#include "../io/fs.h"
#include "../io/pixels.h"

// TEXTURE
Texture::Texture():m_path(""), m_width(0), m_height(0), m_id(0) {
}

Texture::~Texture() {
    clear();
}

void Texture::clear() {
    if (m_id != 0)
        glDeleteTextures(1, &m_id);
    m_id = 0;
}

bool Texture::load(int _width, int _height, int _channels, int _bits, const void* _data) {

    // Generate an OpenGL texture ID for this texturez
    glEnable(GL_TEXTURE_2D);
    if (m_id == 0)
        glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    GLenum format = GL_RGBA;
    if (_channels == 4) {
        format = GL_RGBA;
    }
    else if (_channels == 3) {
        format = GL_RGB;
    }
#if !defined(PLATFORM_RPI) && !defined(PLATFORM_RPI4)
    else if (_channels == 2) {
        format = GL_RG;
    } 
    else if (_channels == 1) {
        format = GL_RED;
    }
#endif
    else
        std::cout << "Unrecognize GLenum format " << _channels << std::endl;

    GLenum type = GL_UNSIGNED_BYTE;
    if (_bits == 32) {
        type = GL_FLOAT;
    }
    else if (_bits == 16) {
        type = GL_UNSIGNED_SHORT;
    } 
    else if (_bits == 8) {
        type = GL_UNSIGNED_BYTE;
    }
    else 
        std::cout << "Unrecognize GLenum type for " << _bits << " bits" << std::endl;

    m_width = _width;
    m_height = _height;
    
#if defined(PLATFORM_RPI) || defined(PLATFORM_RPI4)
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

bool Texture::load(const std::string& _path, bool _vFlip) {
    std::string ext = getExt(_path);

    if (ext == "png"    || ext == "PNG" ||
        ext == "jpg"    || ext == "JPG" ||
        ext == "jpeg"   || ext == "JPEG") {

        unsigned char* pixels = loadPixels(_path, &m_width, &m_height, RGB_ALPHA, _vFlip);
        load(m_width, m_height, 4, 8, pixels);
        // delete[] pixels;
        delete pixels;
    }

    else if (ext == "hdr" || ext == "HDR") {
        float* pixels = loadPixelsHDR(_path, &m_width, &m_height, _vFlip);
        load(m_width, m_height, 3, 32, pixels);
        // delete[] pixels;
        delete pixels;
    }

    m_path = _path;

    // unbind();

    return true;
}

bool Texture::loadBump(const std::string& _path, bool _vFlip) {
    std::string ext = getExt(_path);

    // Generate an OpenGL texture ID for this texture
    glEnable(GL_TEXTURE_2D);
    if (m_id == 0)
        glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    if (ext == "png"    || ext == "PNG" ||
        ext == "jpg"    || ext == "JPG" ||
        ext == "jpeg"   || ext == "JPEG") {

        uint16_t* pixels = loadPixels16(_path, &m_width, &m_height, LUMINANCE, _vFlip);

        const int n = m_width * m_height;
        const float m = 1.f / 65535.f;
        std::vector<float> data;
        data.resize(n);
        for (int i = 0; i < n; i++) {
            data[i] = pixels[i] * m;
        }

        float _scale = -10.0;

        const int w = m_width - 1;
        const int h = m_height - 1;
        std::vector<glm::vec3> result(w * h);
        int i = 0;
        for (int y0 = 0; y0 < h; y0++) {
            const int y1 = y0 + 1;
            const float yc = y0 + 0.5f;
            for (int x0 = 0; x0 < w; x0++) {
                const int x1 = x0 + 1;
                const float xc = x0 + 0.5f;
                const float z00 = data[y0 * w + x0] * -_scale;
                const float z01 = data[y1 * w + x0] * -_scale;
                const float z10 = data[y0 * w + x1] * -_scale;
                const float z11 = data[y1 * w + x1] * -_scale;
                const float zc = (z00 + z01 + z10 + z11) / 4.f;
                const glm::vec3 p00(x0, y0, z00);
                const glm::vec3 p01(x0, y1, z01);
                const glm::vec3 p10(x1, y0, z10);
                const glm::vec3 p11(x1, y1, z11);
                const glm::vec3 pc(xc, yc, zc);
                const glm::vec3 n0 = glm::triangleNormal(pc, p00, p10);
                const glm::vec3 n1 = glm::triangleNormal(pc, p10, p11);
                const glm::vec3 n2 = glm::triangleNormal(pc, p11, p01);
                const glm::vec3 n3 = glm::triangleNormal(pc, p01, p00);
                result[i] = glm::normalize(n0 + n1 + n2 + n3) * 0.5f + 0.5f;
                i++;
            }
        }
        delete pixels;

        load(m_width, m_height, 4, 32, &result[0]);
    }

    m_path = _path;

    return true;
}

void Texture::bind() {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture::unbind() {
    glBindTexture(GL_TEXTURE_2D, 0);
}
