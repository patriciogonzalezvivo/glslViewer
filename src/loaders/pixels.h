#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string>

enum Channels {
    LUMINANCE = 1,
    LUMINANCE_ALPHA = 2,
    RGB = 3,
    RGB_ALPHA = 4
};

// Implementation of savePixels and loadPixels is on gltf.cpp because tiny_gltf.h also use stb_image*.h
bool            savePixels(const std::string& _path, unsigned char* _pixels, int _width, int _height);
unsigned char*  loadPixels(const std::string& _path, int *_width, int *_height, Channels _channels = RGB, bool _vFlip = true);
uint16_t *      loadPixels16(const std::string& _path, int *_width, int *_height, Channels _channels = RGB, bool _vFlip = true);

float*          loadPixelsHDR(const std::string& _path, int *_width, int *_height, bool _vFlip = true);
void            rescalePixels(const unsigned char* _src, int _srcWidth, int _srcHeight, int _srcChannels, int _dstWidth, int _dstHeight, unsigned char* _dst);

template<typename T>
void flipPixelsVertically(T *_pixels, int _width, int _height, int _bytes_per_pixel) {
    const size_t stride = _width * _bytes_per_pixel;
    T *row = (T*)malloc(stride * sizeof(T));
    T *low = _pixels;
    T *high = &_pixels[(_height - 1) * stride];
    for (; low < high; low += stride, high -= stride) {
        memcpy(row, low, stride * sizeof(T));
        memcpy(low, high, stride * sizeof(T));
        memcpy(high, row, stride * sizeof(T));
    }
    free(row);
}
