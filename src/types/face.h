#pragma once

#include <math.h>

#include "gl/gl.h"
#include "glm/glm.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#ifndef M_RPI
#define M_RPI 0.31830988618379067153f
#endif

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

template <typename T> 
struct Face {

    void flipHorizontal() {
        int dataSize = width * height * 3;
        int n = sizeof(T) * 3 * width;
        T* newData = new T[dataSize];

        for (int i = 0; i < height; i++) {
            int offset = i * width * 3;
            int bias = -(i + 1) * 3 * width;

            memcpy(newData + dataSize + bias, data + offset, n);
        }

        delete[] data;
        data = newData;
    }

    void flipVertical() {
        int dataSize = width * height * 3;
        int n = sizeof(T) * 3;
        T* newData = new T[dataSize];

        for(int i = 0; i < height; ++i) {
            int lineOffset = i * width * 3;

            for(int j = 0; j < width; ++j) {
                int offset = lineOffset + j * 3;
                int bias = lineOffset + width * 3 - (j + 1) * 3;

                memcpy(newData + bias, data + offset, n);
            }
        }

        delete[] data;
        data = newData;
    }

    void upload() {
        GLenum type = GL_FLOAT;

        if (sizeof(T) == sizeof(char)) {
            type = GL_UNSIGNED_BYTE;
        }
        
    #ifdef PLATFORM_RPI
        GLenum InternalFormat = GL_RGB;
    #elif defined (PLATFORM_WINDOWS)
        GLenum InternalFormat = GL_RGB16F;
    #else
        GLenum InternalFormat = GL_RGB16F_ARB;
    #endif

        glTexImage2D(CubeMapFace[id], 0, InternalFormat, width, height, 0, GL_RGB, type, data);
    }

    // By @andsz
    // From https://github.com/ands/spherical_harmonics_playground
    int calculateSH(glm::vec3 *_sh) {
        // Calculate SH coefficients:
        int step = 16;
        int samples = 0;
        for (int y = 0; y < height; y += step) {
            T *p = data + y * width * 3;
            for (int x = 0; x < width; x += step) {
                glm::vec3 n = (
                    (   (skyX[id] * ( 2.0f * ((float)x / ((float)width - 1.0f)) - 1.0f)) +
                        (skyY[id] * ( -2.0f * ((float)y / ((float)height - 1.0f)) + 1.0f)) ) +
                    skyDir[id]); // texelDirection;
                float l = glm::length(n);
                glm::vec3 c_light = glm::vec3((float)p[0], (float)p[1], (float)p[2]);
                
                if (sizeof(T) == sizeof(char)) {
                    c_light = c_light / 255.0f;
                }
                    
                c_light = c_light * l * l * l; // texelSolidAngle * texel_radiance;
                n = glm::normalize(n);
                _sh[0] += (c_light * 0.282095f);
                _sh[1] += (c_light * -0.488603f * n.y * 2.0f / 3.0f);
                _sh[2] += (c_light * 0.488603f * n.z * 2.0f / 3.0f);
                _sh[3] += (c_light * -0.488603f * n.x * 2.0f / 3.0f);
                _sh[4] += (c_light * 1.092548f * n.x * n.y / 4.0f);
                _sh[5] += (c_light * -1.092548f * n.y * n.z / 4.0f);
                _sh[6] += (c_light * 0.315392f * (3.0f * n.z * n.z - 1.0f) / 4.0f);
                _sh[7] += (c_light * -1.092548f * n.x * n.z / 4.0f);
                _sh[8] += (c_light * 0.546274f * (n.x * n.x - n.y * n.y) / 4.0f);
                p += 3 * step;
                samples++;
            }
        }
        return samples;
    }

    int     id;
	int     width;
	int     height;
	// for mem copy purposes only
	int     currentOffset;
    T       *data;
};

///
///              +----------+
///              | +---->+x |
///              | |        |
///              | |  +y    |
///              |+z      2 |
///   +----------+----------+----------+----------+
///   | +---->+z | +---->+x | +---->-z | +---->-x |
///   | |        | |        | |        | |        |
///   | |  -x    | |  +z    | |  +x    | |  -z    |
///   |-y      1 |-y      4 |-y      0 |-y      5 |
///   +----------+----------+----------+----------+
///              | +---->+x |
///              | |        |
///              | |  -y    |
///              |-z      3 |
///              +----------+
///
static const float s_faceUvVectors[6][3][3] =
{
    { // +x face
        {  0.0f,  0.0f, -1.0f }, // u -> -z
        {  0.0f, -1.0f,  0.0f }, // v -> -y
        {  1.0f,  0.0f,  0.0f }, // +x face
    },
    { // -x face
        {  0.0f,  0.0f,  1.0f }, // u -> +z
        {  0.0f, -1.0f,  0.0f }, // v -> -y
        { -1.0f,  0.0f,  0.0f }, // -x face
    },
    { // +y face
        {  1.0f,  0.0f,  0.0f }, // u -> +x
        {  0.0f,  0.0f,  1.0f }, // v -> +z
        {  0.0f,  1.0f,  0.0f }, // +y face
    },
    { // -y face
        {  1.0f,  0.0f,  0.0f }, // u -> +x
        {  0.0f,  0.0f, -1.0f }, // v -> -z
        {  0.0f, -1.0f,  0.0f }, // -y face
    },
    { // +z face
        {  1.0f,  0.0f,  0.0f }, // u -> +x
        {  0.0f, -1.0f,  0.0f }, // v -> -y
        {  0.0f,  0.0f,  1.0f }, // +z face
    },
    { // -z face
        { -1.0f,  0.0f,  0.0f }, // u -> -x
        {  0.0f, -1.0f,  0.0f }, // v -> -y
        {  0.0f,  0.0f, -1.0f }, // -z face
    }
};

/// _u and _v should be center adressing and in [-1.0+invSize..1.0-invSize] range.
static inline void texelCoordToVec(float* _out3f, float _u, float _v, uint8_t _faceId) {
    // out = u * s_faceUv[0] + v * s_faceUv[1] + s_faceUv[2].
    _out3f[0] = s_faceUvVectors[_faceId][0][0] * _u + s_faceUvVectors[_faceId][1][0] * _v + s_faceUvVectors[_faceId][2][0];
    _out3f[1] = s_faceUvVectors[_faceId][0][1] * _u + s_faceUvVectors[_faceId][1][1] * _v + s_faceUvVectors[_faceId][2][1];
    _out3f[2] = s_faceUvVectors[_faceId][0][2] * _u + s_faceUvVectors[_faceId][1][2] * _v + s_faceUvVectors[_faceId][2][2];

    // Normalize.
    const float invLen = 1.0f/sqrtf(_out3f[0]*_out3f[0] + _out3f[1]*_out3f[1] + _out3f[2]*_out3f[2]);
    _out3f[0] *= invLen;
    _out3f[1] *= invLen;
    _out3f[2] *= invLen;
}

static inline void latLongFromVec(float& _u, float& _v, const float _vec[3]) {
    const float phi = atan2f(_vec[0], _vec[2]);
    const float theta = acosf(_vec[1]);

    _u = (M_PI + phi) * (0.5f / M_PI);
    _v = theta * M_RPI;
}

static inline uint32_t ftou(float _f) { 
    return uint32_t(int32_t(_f)); 
}

template <typename T> 
inline void vec3Mul(T* __restrict _result, const T* __restrict _a, float _b) {
	_result[0] = _a[0] * _b;
	_result[1] = _a[1] * _b;
	_result[2] = _a[2] * _b;
}
