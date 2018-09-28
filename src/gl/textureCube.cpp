#include <iostream>
#include <cstring>

#include "textureCube.h"

#include "tools/fs.h"
#include "tools/face.h"
#include "tools/image.h"


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif 

#ifndef M_2_SQRTPI
#define M_2_SQRTPI 1.12837916709551257390
#endif

#ifndef M_MIN
#define M_MIN(_a, _b) ((_a)<(_b)?(_a):(_b))
#endif

#define USE_BILINEAR_INTERPOLATION

// A few useful utilities from Filament
// https://github.com/google/filament/blob/master/tools/cmgen/src/CubemapSH.cpp
// -----------------------------------------------------------------------------------------------

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
        _faces[i]->id = i;
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
        _faces[i]->id = i;
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

// From
// https://github.com/dariomanesku/cmft/blob/master/src/cmft/image.cpp#L3124
template <typename T> 
void splitFacesFromEquilateral(T *_data, unsigned int _width, unsigned int _height, Face<T> **_faces ) {
    // Alloc data.
    const uint32_t faceWidth = (_height + 1)/2;
    const uint32_t faceHeight = faceWidth;

    // Get source parameters.
    const float srcWidthMinusOne  = float(int(_width-1));
    const float srcHeightMinusOne = float(int(_height-1));
    const float invfaceWidthf = 1.0f/float(faceWidth);

    for (int i = 0; i < 6; i++) {
        _faces[i] = new Face<T>();
        _faces[i]->id = i;
        _faces[i]->data = new T[3 * faceWidth * faceHeight];
        _faces[i]->width = faceWidth;
        _faces[i]->height = faceHeight;
        _faces[i]->currentOffset = 0;

        for (uint32_t yy = 0; yy < faceHeight; ++yy) {
            T* dstRowData = &_faces[i]->data[yy * faceWidth * 3];

            for (uint32_t xx = 0; xx < faceWidth; ++xx) {
                T* dstColumnData = &dstRowData[xx * 3];

                // Cubemap (u,v) on current face.
                const float uu = 2.0f*xx*invfaceWidthf-1.0f;
                const float vv = 2.0f*yy*invfaceWidthf-1.0f;

                // Get cubemap vector (x,y,z) from (u,v,faceIdx).
                float vec[3];
                texelCoordToVec(vec, uu, vv, i);

                // Convert cubemap vector (x,y,z) to latlong (u,v).
                float xSrcf;
                float ySrcf;
                latLongFromVec(xSrcf, ySrcf, vec);

                // Convert from [0..1] to [0..(size-1)] range.
                xSrcf *= srcWidthMinusOne;
                ySrcf *= srcHeightMinusOne;

                // Sample from latlong (u,v).
                #ifdef USE_BILINEAR_INTERPOLATION
                    const uint32_t x0 = ftou(xSrcf);
                    const uint32_t y0 = ftou(ySrcf);
                    const uint32_t x1 = M_MIN(x0+1, _width-1);
                    const uint32_t y1 = M_MIN(y0+1, _height-1);

                    const T *src0 = &_data[y0 * _width * 3 + x0 * 3];
                    const T *src1 = &_data[y0 * _width * 3 + x1 * 3];
                    const T *src2 = &_data[y1 * _width * 3 + x0 * 3];
                    const T *src3 = &_data[y1 * _width * 3 + x1 * 3];

                    const float tx = xSrcf - float(int(x0));
                    const float ty = ySrcf - float(int(y0));
                    const float invTx = 1.0f - tx;
                    const float invTy = 1.0f - ty;

                    T p0[3];
                    T p1[3];
                    T p2[3];
                    T p3[3];
                    vec3Mul(p0, src0, invTx*invTy);
                    vec3Mul(p1, src1,    tx*invTy);
                    vec3Mul(p2, src2, invTx*   ty);
                    vec3Mul(p3, src3,    tx*   ty);

                    const T rr = p0[0] + p1[0] + p2[0] + p3[0];
                    const T gg = p0[1] + p1[1] + p2[1] + p3[1];
                    const T bb = p0[2] + p1[2] + p2[2] + p3[2];

                    dstColumnData[0] = rr;
                    dstColumnData[1] = gg;
                    dstColumnData[2] = bb;
                #else
                    const uint32_t xSrc = ftou(xSrcf);
                    const uint32_t ySrc = ftou(ySrcf);

                    dstColumnData[0] = _data[ySrc * _width * 3 + xSrc * 3 + 0];
                    dstColumnData[1] = _data[ySrc * _width * 3 + xSrc * 3 + 1];
                    dstColumnData[2] = _data[ySrc * _width * 3 + xSrc * 3 + 2];
                #endif
            }
        }
    }
}

bool TextureCube::load(const std::string &_path, bool _vFlip) {

    // Init
    glGenTextures(1, &m_id);

    glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
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
            if (m_width/2 == m_height)  {
                splitFacesFromEquilateral<unsigned char>(data, m_width, m_height, faces);
            }
            else {
                splitFacesHorizontal<unsigned char>(data, m_width, m_height, faces);
            }
        }
        
        for (int i = 0; i < 6; i++) {
            faces[i]->upload();
            sh_samples += faces[i]->calculateSH(SH);
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

            if (m_width/2 == m_height)  {
                splitFacesFromEquilateral<float>(data, m_width, m_height, faces);
            }
            else {
                splitFacesHorizontal<float>(data, m_width, m_height, faces);
            }
        }

        for (int i = 0; i < 6; i++) {
            faces[i]->upload();
            sh_samples += faces[i]->calculateSH(SH);
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
    }

    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    m_path = _path;             
    return true;
}

void TextureCube::bind() {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);
}
