#include <iostream>
#include <cstring>

#include "textureCube.h"

TextureCube::TextureCube() {
}

TextureCube::~TextureCube() {
    glDeleteTextures(1, &m_id);

    for(int i = 0; i < 6; ++i) {
        delete[] m_faces[i]->data;
        delete m_faces[i];
    }

    delete[] m_faces;
}

bool TextureCube::load(const std::string &_path, bool _vFlip) {
    std::cout << "Loading hdr texture " << _path << ".." << std::endl;

    FILE* file = fopen(_path.c_str(), "rb");

    RGBE_ReadHeader(file, &m_width, &m_height, NULL);
    float* data = new float[3 * m_width * m_height];
    RGBE_ReadPixels_RLE(file, data, m_width, m_height);

    fclose(file);

    // LOAD FACES
    m_faces = new Face*[6];

    int faceWidth = m_width / 3;
    int faceHeight = m_height / 4;

    for (int i = 0; i < 6; ++i) {
        m_faces[i] = new Face();
        m_faces[i]->data = new float[3 * faceWidth * faceHeight];
        m_faces[i]->width = faceWidth;
        m_faces[i]->height = faceHeight;
        m_faces[i]->currentOffset = 0;
    }

    for (int l = 0; l < m_height; ++l) {
        int jFace = (l - (l % faceHeight)) / faceHeight;

        for (int iFace = 0; iFace < 3; ++iFace) {
            Face *face = NULL;
            int offset = 3 * (faceWidth * iFace + l * m_width);

            if (iFace == 2 && jFace == 1) face = m_faces[0]; // POS_Y
            if (iFace == 0 && jFace == 1) face = m_faces[1]; // NEG_Y
            if (iFace == 1 && jFace == 0) face = m_faces[2]; // POS_X
            if (iFace == 1 && jFace == 2) face = m_faces[3]; // NEG_X
            if (iFace == 1 && jFace == 1) face = m_faces[4]; // POS_Z
            if (iFace == 1 && jFace == 3) face = m_faces[5]; // NEG_Z

            if (face) {
                // the number of components to copy
                int n = sizeof(float) * faceWidth * 3;

                std::memcpy(face->data + face->currentOffset, data + offset, n);
                face->currentOffset += (3 * faceWidth);
            }
        }
    }

    // adjust NEG_Z face
    if (_vFlip) {
        _flipHorizontal(m_faces[5]);
        _flipVertical(m_faces[5]);
    }

    // Init
    glGenTextures(1, &m_id);

    glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#ifndef PLATFORM_RPI
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
#endif

    GLenum Format = GL_RGB;
    GLenum Type = GL_FLOAT;
#ifndef PLATFORM_RPI
    GLenum InternalFormat = GL_RGB16F_ARB;

    for(int i = 0; i < 6; ++i) {
        Face *f = m_faces[i];
        glTexImage2D(CubeMapFace[i], 0, InternalFormat, f->width, f->height, 0, Format, Type, f->data);
    }
 #else
    GLenum InternalFormat = GL_RGB;
    for(int i = 0; i < 6; ++i) {
        Face *f = m_faces[i];
        int total = 3 * faceWidth * faceHeight;
        unsigned char pixels[total];
        
        for (int j = 0; j < total; i++) {
            pixels[i] = (int)(f->data[i] * 255);
        }
        glTexImage2D(CubeMapFace[i], 0, InternalFormat, f->width, f->height, 0, Format, Type, pixels);
        //delete[] pixels;
    }
 #endif

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    // std::cout << "// HDR Texture parameters : " << std::endl;
    // std::cout << "//  - Format: " << ((Format == GL_RGB) ? "RGB" : "") << std::endl;
    // std::cout << "//  - Type : " << ((Type == GL_FLOAT) ? "Float" : "") << std::endl;
    // std::cout << "//  - Internal Format : " << ((InternalFormat == GL_RGB8) ? "RGB8" : "RGB16F") << std::endl;

    // // Debug
    // std::cout << "//  - width : " << m_width << "px" << std::endl;
    // std::cout << "//  - height : " << m_height << "px" << std::endl;
    // std::cout << "//  - memory size : " << (3 * m_width * m_height * sizeof(float)) / 8 << " bytes" << std::endl;
    // std::cout << "// Generating texture cube.." << std::endl;

    m_path = _path;

    delete[] data;
    
    return true;
}

void TextureCube::_flipHorizontal(Face* face) {
    int dataSize = face->width * face->height * 3;
    int n = sizeof(float) * 3 * face->width;
    float* newData = new float[dataSize];

    for (int i = 0; i < face->height; i++) {
        int offset = i * face->width * 3;
        int bias = -(i + 1) * 3 * face->width;

        memcpy(newData + dataSize + bias, face->data + offset, n);
    }

    delete[] face->data;
    face->data = newData;
}

void TextureCube::_flipVertical(Face* face) {
    int dataSize = face->width * face->height * 3;
    int n = sizeof(float) * 3;
    float* newData = new float[dataSize];

    for(int i = 0; i < face->height; ++i) {
        int lineOffset = i * face->width * 3;

        for(int j = 0; j < face->width; ++j) {
            int offset = lineOffset + j * 3;
            int bias = lineOffset + face->width * 3 - (j + 1) * 3;

            memcpy(newData + bias, face->data + offset, n);
        }
    }

    delete[] face->data;
    face->data = newData;
}

void TextureCube::bind() {
    // glActiveTexture(GL_TEXTURE0 + textureIndex);
    // glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);
    // return m_id;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);
}
