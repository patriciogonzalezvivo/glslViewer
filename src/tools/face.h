#pragma once

#include "gl/gl.h"
#include "glm/glm.hpp"

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

    void upload(GLenum _face) {
        GLenum type = GL_FLOAT;

        if (sizeof(T) == sizeof(char)) {
            type = GL_UNSIGNED_BYTE;
        }
        
    #ifndef PLATFORM_RPI
        GLenum InternalFormat = GL_RGB16F_ARB;
    #else
        GLenum InternalFormat = GL_RGB;
    #endif

        glTexImage2D(_face, 0, InternalFormat, width, height, 0, GL_RGB, type, data);
    }

    // By @andsz
    // From https://github.com/ands/spherical_harmonics_playground
    int calculateSH(const glm::vec3 &_skyDir, const glm::vec3 &_skyX, const glm::vec3 &_skyY, glm::vec3 *_sh) {
        // Calculate SH coefficients:
        int step = 16;
        int samples = 0;
        for (int y = 0; y < height; y += step) {
            T *p = data + y * width * 3;
            for (int x = 0; x < width; x += step) {
                glm::vec3 n = (
                    (   (_skyX * ( 2.0f * ((float)x / ((float)width - 1.0f)) - 1.0f)) +
                        (_skyY * ( -2.0f * ((float)y / ((float)height - 1.0f)) + 1.0f)) ) +
                    _skyDir); // texelDirection;
                float l = glm::length(n);
                glm::vec3 c_light = glm::vec3((float)p[0], (float)p[1], (float)p[2]);
                
                if (sizeof(T) == sizeof(char)) {
                    c_light = c_light / 255.0f;
                }
                    
                c_light = c_light * l * l * l; // texelSolidAngle * texel_radiance;
                n = glm::normalize(n);
                _sh[0] = (_sh[0] + (c_light * 0.282095f));
                _sh[1] = (_sh[1] + (c_light * -0.488603f * n.y * 2.0f / 3.0f));
                _sh[2] = (_sh[2] + (c_light * 0.488603f * n.z * 2.0f / 3.0f));
                _sh[3] = (_sh[3] + (c_light * -0.488603f * n.x * 2.0f / 3.0f));
                _sh[4] = (_sh[4] + (c_light * 1.092548f * n.x * n.y / 4.0f));
                _sh[5] = (_sh[5] + (c_light * -1.092548f * n.y * n.z / 4.0f));
                _sh[6] = (_sh[6] + (c_light * 0.315392f * (3.0f * n.z * n.z - 1.0f) / 4.0f));
                _sh[7] = (_sh[7] + (c_light * -1.092548f * n.x * n.z / 4.0f));
                _sh[8] = (_sh[8] + (c_light * 0.546274f * (n.x * n.x - n.y * n.y) / 4.0f));
                p += 3 * step;
                samples++;
            }
        }
        return samples;
    }

	int width;
	int height;
	// for mem copy purposes only
	int currentOffset;
    T   *data;
};