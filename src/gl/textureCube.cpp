#include <iostream>
#include <cstring>

#include "textureCube.h"

#include "tools/fs.h"
#include "tools/face.h"
#include "tools/image.h"

const GLenum CubeMapFace[6] { 
    GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z 
};

const glm::vec3 skyDir[] = {
    glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
    glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f)
};
const glm::vec3 skyX[] = {
    glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, 1.0f),
    glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
    glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f)
};
const glm::vec3 skyY[] = {
    glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
    glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, 1.0f),
    glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)
};

TextureCube::TextureCube() 
: SH {  glm::vec3(0.0), glm::vec3(0.0), glm::vec3(0.0),
        glm::vec3(0.0), glm::vec3(0.0), glm::vec3(0.0),
        glm::vec3(0.0), glm::vec3(0.0), glm::vec3(0.0) } {
}

TextureCube::~TextureCube() {
    glDeleteTextures(1, &m_id);
}

template <typename T> 
void splitFacesVertical(T *_data, int _width, int _height, Face<T> **_faces ) {
    int faceWidth = _width / 3;
    int faceHeight = _height / 4;

    for (int i = 0; i < 6; i++) {
        _faces[i] = new Face<T>();
        _faces[i]->data = new T[3 * faceWidth * faceHeight];
        _faces[i]->width = faceWidth;
        _faces[i]->height = faceHeight;
        _faces[i]->currentOffset = 0;
    }

    for (int l = 0; l < _height; l++) {
        int jFace = (l - (l % faceHeight)) / faceHeight;

        for (int iFace = 0; iFace < 3; iFace++) {
            Face<T> *face = NULL;
            int offset = 3 * (faceWidth * iFace + l * _width);

            //      0   1   2   i
            //  3      -Z       
            //  2      -X 
            //  1  -Y  +Z  +Y       
            //  0      +X
            //  j
            //
            if (iFace == 2 && jFace == 1) face = _faces[0]; // POS_Y
            if (iFace == 0 && jFace == 1) face = _faces[1]; // NEG_Y
            if (iFace == 1 && jFace == 0) face = _faces[2]; // POS_X
            if (iFace == 1 && jFace == 2) face = _faces[3]; // NEG_X
            if (iFace == 1 && jFace == 1) face = _faces[4]; // POS_Z
            if (iFace == 1 && jFace == 3) face = _faces[5]; // NEG_Z

            if (face) {
                // the number of components to copy
                int n = sizeof(T) * faceWidth * 3;

                std::memcpy(face->data + face->currentOffset, _data + offset, n);
                face->currentOffset += (3 * faceWidth);
            }
        }
    }
}

template <typename T> 
void splitFacesHorizontal(T *_data, int _width, int _height, Face<T> **_faces ) {
    int faceWidth = _width / 4;
    int faceHeight = _height / 3;

    for (int i = 0; i < 6; i++) {
        _faces[i] = new Face<T>();
        _faces[i]->data = new T[3 * faceWidth * faceHeight];
        _faces[i]->width = faceWidth;
        _faces[i]->height = faceHeight;
        _faces[i]->currentOffset = 0;
    }

    for (int l = 0; l < _height; l++) {
        int jFace = (l - (l % faceHeight)) / faceHeight;

        for (int iFace = 0; iFace < 4; iFace++) {
            Face<T> *face = NULL;
            int offset = 3 * (faceWidth * iFace + l * _width);

            //      0   1   2   3 i      
            //  2      -X 
            //  1  -Y  +Z  +Y  -Z     
            //  0      +X
            //  j
            //
            if (iFace == 2 && jFace == 1) face = _faces[0]; // POS_Y
            if (iFace == 0 && jFace == 1) face = _faces[1]; // NEG_Y
            if (iFace == 1 && jFace == 0) face = _faces[2]; // POS_X
            if (iFace == 1 && jFace == 2) face = _faces[3]; // NEG_X
            if (iFace == 1 && jFace == 1) face = _faces[4]; // POS_Z
            if (iFace == 3 && jFace == 1) face = _faces[5]; // NEG_Z

            if (face) {
                // the number of components to copy
                int n = sizeof(T) * faceWidth * 3;

                std::memcpy(face->data + face->currentOffset, _data + offset, n);
                face->currentOffset += (3 * faceWidth);
            }
        }
    }
}

bool TextureCube::load(const std::string &_path, bool _vFlip) {

    // Init
    glGenTextures(1, &m_id);

    glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#ifndef PLATFORM_RPI
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
#endif

    int sh_samples = 0;
    if (haveExt(_path,"png") || haveExt(_path,"PNG") ||
        haveExt(_path,"jpg") || haveExt(_path,"JPG") ||
        haveExt(_path,"jpeg") || haveExt(_path,"JPEG")) {

        unsigned char* data = loadPixels(_path, &m_width, &m_height, RGB, false);

        // LOAD FACES
        Face<unsigned char> **faces = new Face<unsigned char>*[6];

        if (m_height > m_width) {
            splitFacesVertical<unsigned char>(data, m_width, m_height, faces);

            // adjust NEG_Z face
            if (_vFlip) {
                faces[5]->flipHorizontal();
                faces[5]->flipVertical();
            }
        }
        else {
            splitFacesHorizontal<unsigned char>(data, m_width, m_height, faces);
        }
        
        for (int i = 0; i < 6; i++) {
            faces[i]->upload(CubeMapFace[i]);
            sh_samples += faces[i]->calculateSH(skyDir[i], skyX[i], skyY[i], SH);
        }

        delete[] data;
        for(int i = 0; i < 6; ++i) {
            delete[] faces[i]->data;
            delete faces[i];
        }
        delete[] faces;

    }

    else if (haveExt(_path, "hdr") || haveExt(_path,"HDR")) {
        float* data = loadFloatPixels(_path, &m_width, &m_height, false);

        // LOAD FACES
        Face<float> **faces = new Face<float>*[6];

        if (m_height > m_width) {
            splitFacesVertical<float>(data, m_width, m_height, faces);

            // adjust NEG_Z face
            if (_vFlip) {
                faces[5]->flipHorizontal();
                faces[5]->flipVertical();
            }
        }
        else {
            splitFacesHorizontal<float>(data, m_width, m_height, faces);
        }

        for (int i = 0; i < 6; i++) {
            faces[i]->upload(CubeMapFace[i]);
            sh_samples += faces[i]->calculateSH(skyDir[i], skyX[i], skyY[i], SH);
        }

        delete[] data;
        for(int i = 0; i < 6; ++i) {
            delete[] faces[i]->data;
            delete faces[i];
        }
        delete[] faces;

    }

    for (int i = 0; i < 9; i++) {
        SH[i] = SH[i] * (32.0f / (float)sh_samples);
        // std::cout << SH[i].x << "," << SH[i].y << "," << SH[i].z << std::endl;
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    m_path = _path;             
    return true;
}

void TextureCube::bind() {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);
}
