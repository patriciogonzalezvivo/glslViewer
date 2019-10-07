#include <iostream>
#include <cstring>

#include "textureCube.h"

#include "../tools/fs.h"
#include "../tools/face.h"
#include "../tools/math.h"
#include "../loaders/pixels.h"

// #define USE_BILINEAR_INTERPOLATION

// extern "C" {
#include "skylight/ArHosekSkyModel.h"
// }

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
void splitFacesFromVerticalCross(T *_data, int _width, int _height, Face<T> **_faces ) {
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
void splitFacesFromHorizontalCross(T *_data, int _width, int _height, Face<T> **_faces ) {
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

template <typename T> 
void splitFacesFromHorizontalRow(T *_data, int _width, int _height, Face<T> **_faces ) {
    int faceWidth = _width / 6;
    int faceHeight = _height;

    for (int i = 0; i < 6; i++) {
        _faces[i] = new Face<T>();
        _faces[i]->id = i;
        _faces[i]->data = new T[3 * faceWidth * faceHeight];
        _faces[i]->width = faceWidth;
        _faces[i]->height = faceHeight;
        _faces[i]->currentOffset = 0;
    }

    for (int l = 0; l < _height; l++) {
        // int jFace = (l - (l % faceHeight)) / faceHeight;
        for (int iFace = 0; iFace < 6; iFace++) {
            Face<T> *face = NULL;
            int offset = 3 * (faceWidth * iFace + l * _width);

            //   0   1   2   3   4   5 i      
            //  +X  -X  +Y  -Y  +Z  -Z 
            //
            face = _faces[iFace];

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
void splitFacesFromVerticalRow(T *_data, int _width, int _height, Face<T> **_faces ) {
    int faceWidth = _width;
    int faceHeight = _height/6;

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
        for (int iFace = 0; iFace < 6; iFace++) {
            Face<T> *face = NULL;
            int offset = 3 * (faceWidth * iFace + l * _width);

            //   0   1   2   3   4   5 j      
            //  +X  -X  +Y  -Y  +Z  -Z 
            //
            face = _faces[jFace];

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
    std::string ext = getExt(_path);

    if (m_id != 0) {
        // Init
        glGenTextures(1, &m_id);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);
    }

    int sh_samples = 0;
    if (ext == "png"    || ext == "PNG" ||
        ext == "jpg"    || ext == "JPG" ||
        ext == "jpeg"   || ext == "JPEG" ) {

        unsigned char* data = loadPixels(_path, &m_width, &m_height, RGB, false);

        // LOAD FACES
        Face<unsigned char> **faces = new Face<unsigned char>*[6];

        if (m_height > m_width) {

            if (m_width/6 == m_height) {
                // Vertical Row
                splitFacesFromVerticalRow<unsigned char>(data, m_width, m_height, faces);
            }
            else {
                // Vertical Cross
                splitFacesFromVerticalCross<unsigned char>(data, m_width, m_height, faces);

                // adjust NEG_Z face
                if (_vFlip) {
                    faces[5]->flipHorizontal();
                    faces[5]->flipVertical();
                }
            }
            
        }
        else {
            if (m_width/2 == m_height) {
                // Equilateral
                splitFacesFromEquilateral<unsigned char>(data, m_width, m_height, faces);
            }
            else if (m_width/6 == m_height) {
                // Horizontal Row
                splitFacesFromHorizontalRow<unsigned char>(data, m_width, m_height, faces);
            }
            else {
                // Horizontal Cross
                splitFacesFromHorizontalCross<unsigned char>(data, m_width, m_height, faces);
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

    else if (ext == "hdr" || ext == "HDR") {
        float* data = loadPixelsHDR(_path, &m_width, &m_height, false);

        // LOAD FACES
        Face<float> **faces = new Face<float>*[6];

        if (m_height > m_width) {
            if (m_width/6 == m_height) {
                // Vertical Row
                splitFacesFromVerticalRow<float>(data, m_width, m_height, faces);
            }
            else {
                // Vertical Cross
                splitFacesFromVerticalCross<float>(data, m_width, m_height, faces);

                // adjust NEG_Z face
                if (_vFlip) {
                    faces[5]->flipHorizontal();
                    faces[5]->flipVertical();
                }
            }
        }
        else {

            if (m_width/2 == m_height)  {
                // Equilatera
                splitFacesFromEquilateral<float>(data, m_width, m_height, faces);
            }
            else if (m_width/6 == m_height) {
                // Horizontal Row
                splitFacesFromHorizontalRow<float>(data, m_width, m_height, faces);
            }
            else {
                // Horizontal Cross
                splitFacesFromHorizontalCross<float>(data, m_width, m_height, faces);
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

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#if !defined(PLATFORM_RPI) && !defined(PLATFORM_RPI4) 
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
#endif
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    m_path = _path;             
    return true;
}

bool TextureCube::generate(SkyBox* _skybox, int _width ) {

    if (m_id != 0) {
        // Init
        glGenTextures(1, &m_id);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);
    }

    int sh_samples = 0;

    m_width = _width;
    m_height = int(_width/2);
    int nPixels = m_width * m_height * 3;

    float *data = new float[nPixels]; 

    // FILAMENT SKYGEN 
    // https://github.com/google/filament/blob/master/tools/skygen/src/main.cpp
    //
    float solarElevation = _skybox->elevation;
    float sunTheta = float(M_PI_2 - solarElevation);
    float sunPhi = 0.0f;
    bool normalize = true;

    ArHosekSkyModelState* skyState[9] = {
        arhosek_xyz_skymodelstate_alloc_init(_skybox->turbidity, _skybox->groundAlbedo.r, solarElevation),
        arhosek_xyz_skymodelstate_alloc_init(_skybox->turbidity, _skybox->groundAlbedo.g, solarElevation),
        arhosek_xyz_skymodelstate_alloc_init(_skybox->turbidity, _skybox->groundAlbedo.b, solarElevation)
    };

    glm::mat3 XYZ_sRGB = glm::mat3(
            3.2404542f, -0.9692660f,  0.0556434f,
            -1.5371385f,  1.8760108f, -0.2040259f,
            -0.4985314f,  0.0415560f,  1.0572252f
    );

    float maxSample = 0.00001f;
    for (int y = 0; y < m_height; y++) {
        float v = (y + 0.5f) / m_height;
        float theta = float(M_PI * v);

        if (theta > M_PI_2) 
            continue;
            
        for (int x = 0; x < m_width; x++) {
            float u = (x + 0.5f) / m_width;
            float phi = float(-2.0 * M_PI * u + M_PI + _skybox->azimuth);

            float gamma = angleBetween(theta, phi, sunTheta, sunPhi);

            glm::vec3 sample = glm::vec3(
                arhosek_tristim_skymodel_radiance(skyState[0], theta, gamma, 0),
                arhosek_tristim_skymodel_radiance(skyState[1], theta, gamma, 1),
                arhosek_tristim_skymodel_radiance(skyState[2], theta, gamma, 2)
            );

            if (normalize) {
                sample *= float(4.0 * M_PI / 683.0);
            }

            maxSample = std::max(maxSample, sample.y);
            sample = XYZ_sRGB * sample;

            int index = (y * m_width * 3) + x * 3;
            data[index] = sample.r;
            data[index + 1] = sample.g;
            data[index + 2] = sample.b;
        }
    }

    // cleanup sky data
    arhosekskymodelstate_free(skyState[0]);
    arhosekskymodelstate_free(skyState[1]);
    arhosekskymodelstate_free(skyState[2]);

    float hdrScale = 1.0f / (normalize ? maxSample : maxSample / 16.0f);

    for (int y = 0; y < m_height; y++) {
        for (int x = 0; x < m_width; x++) {
            int i = (y * m_width) + x;
            i *= 3;

            if (y >= m_height / 2) {
                data[i + 0] = _skybox->groundAlbedo.r;
                data[i + 1] = _skybox->groundAlbedo.g;
                data[i + 2] = _skybox->groundAlbedo.b;
            }
            else {
                data[i + 0] *= hdrScale;
                data[i + 1] *= hdrScale;
                data[i + 2] *= hdrScale;
            }
        }
    }

    // LOAD FACES
    Face<float> **faces = new Face<float>*[6];
    splitFacesFromEquilateral<float>(data, m_width, m_height, faces);
    
    for (int i = 0; i < 6; i++) {
        faces[i]->upload();
        sh_samples += faces[i]->calculateSH(SH);
    }

    for(int i = 0; i < 6; ++i) {
        delete[] faces[i]->data;
        delete faces[i];
    }
    delete[] faces;
    // delete[] data;

    for (int i = 0; i < 9; i++) {
        SH[i] = SH[i] * (32.0f / (float)sh_samples);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#if !defined(PLATFORM_RPI) && !defined(PLATFORM_RPI4) 
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
#endif
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    return true;
}

void TextureCube::bind() {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);
}
