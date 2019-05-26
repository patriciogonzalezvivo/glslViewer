#include <iostream>

#include "texture.h"

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
        float* pixels = loadFloatPixels(_path, &m_width, &m_height, _vFlip);

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

void Texture::bind() {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture::unbind() {
    glBindTexture(GL_TEXTURE_2D, 0);
}
