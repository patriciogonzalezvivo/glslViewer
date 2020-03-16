#pragma once

#include <stdlib.h>
#include <cstring>
#include <string>

enum Channels {
    LUMINANCE = 1,
    LUMINANCE_ALPHA = 2,
    RGB = 3,
    RGB_ALPHA = 4
};

// Implementation of savePixels and loadPixels is on gltf.cpp because tiny_gltf.h also use stb_image*.h
bool            savePixels(const std::string& _path, unsigned char* _pixels, int _width, int _height);
bool            savePixels16(const std::string& _path, unsigned short* _pixels, int _width, int _height);
bool            savePixelsHDR(const std::string& _path, float* _pixels, int _width, int _height);

unsigned char*  loadPixels(const std::string& _path, int *_width, int *_height, Channels _channels = RGB, bool _vFlip = true);
uint16_t *      loadPixels16(const std::string& _path, int *_width, int *_height, Channels _channels = RGB, bool _vFlip = true);
float*          loadPixelsHDR(const std::string& _path, int *_width, int *_height, bool _vFlip = true);

template<typename T>
void rescalePixels(const T* _src, int _srcWidth, int _srcHeight, int _srcChannels, int _dstWidth, int _dstHeight, T* _dst) {
    int x, y, i, c;
    int x_src, y_src, i_src;
    for (y = 0; y < _dstHeight; y++) {
        for (x = 0; x < _dstWidth; x++) {
            i = y * _dstWidth + x;
            y_src = ((float)(y)/(float)(_dstHeight-1)) * (_srcHeight-1);
            x_src = ((float)(x)/(float)(_dstWidth-1)) * (_srcWidth-1);
            i_src = y_src * _srcWidth + x_src;

            i *= _srcChannels;
            i_src *= _srcChannels;

            for (c = 0; c < _srcChannels; c++) {
                _dst[i + c] = _src[i_src + c];
            }
        }
    }
}
template<typename T>
void flipPixelsVertically(T *_pixels, int _width, int _height, int _bytes_per_pixel) {
    const size_t stride = _width * _bytes_per_pixel;
    T *row = (T*)malloc(stride * sizeof(T));
    T *low = _pixels;
    T *high = &_pixels[(_height - 1) * stride];
    for (; low < high; low += stride, high -= stride) {
        std::memcpy(row, low, stride * sizeof(T));
        std::memcpy(low, high, stride * sizeof(T));
        std::memcpy(high, row, stride * sizeof(T));
    }
    free(row);
}
