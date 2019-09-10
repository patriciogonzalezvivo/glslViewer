#include <iostream>

#include "texture.h"

#include "glm/glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/normal.hpp>

#include "tools/fs.h"
#include "tools/image.h"

// TEXTURE
Texture::Texture():m_path(""), m_width(0), m_height(0), m_id(0) {
}

Texture::~Texture() {
    glDeleteTextures(1, &m_id);
}

bool Texture::load(const std::string& _path, bool _vFlip) {
    glEnable(GL_TEXTURE_2D);

    if (m_id == 0){
        // Generate an OpenGL texture ID for this texture
        glGenTextures(1, &m_id);
    }

    glBindTexture(GL_TEXTURE_2D, m_id);

    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    if (haveExt(_path,"png") || haveExt(_path,"PNG") ||
        haveExt(_path,"jpg") || haveExt(_path,"JPG") ||
        haveExt(_path,"jpeg") || haveExt(_path,"JPEG")) {

        unsigned char* pixels = loadPixels(_path, &m_width, &m_height, RGB_ALPHA, _vFlip);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

        // delete[] pixels;
        delete pixels;
    }

    else if (haveExt(_path, "hdr") || haveExt(_path,"HDR")) {
        float* pixels = loadHDRFloatPixels(_path, &m_width, &m_height, _vFlip);

    #ifndef PLATFORM_RPI
        GLenum InternalFormat = GL_RGB16F_ARB;
    #else
        GLenum InternalFormat = GL_RGB;
    #endif

        glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, m_width, m_height, 0, GL_RGB, GL_FLOAT, pixels);

        // delete[] pixels;
        delete pixels;
    }

    glGenerateMipmap(GL_TEXTURE_2D);

    m_path = _path;

    unbind();

    return true;
}

bool Texture::loadBump(const std::string& _path, bool _vFlip) {
    glEnable(GL_TEXTURE_2D);

    if (m_id == 0){
        // Generate an OpenGL texture ID for this texture
        glGenTextures(1, &m_id);
    }

    glBindTexture(GL_TEXTURE_2D, m_id);

    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    if (haveExt(_path,"png") || haveExt(_path,"PNG") ||
        haveExt(_path,"jpg") || haveExt(_path,"JPG") ||
        haveExt(_path,"jpeg") || haveExt(_path,"JPEG")) {

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

    #ifndef PLATFORM_RPI
        GLenum InternalFormat = GL_RGB16F_ARB;
    #else
        GLenum InternalFormat = GL_RGB;
    #endif

        glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, m_width, m_height, 0, GL_RGB, GL_FLOAT, &result[0]);

        delete pixels;
    }

    glGenerateMipmap(GL_TEXTURE_2D);

    m_path = _path;

    unbind();

    return true;
}

void Texture::bind() {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture::unbind() {
    glBindTexture(GL_TEXTURE_2D, 0);
}
