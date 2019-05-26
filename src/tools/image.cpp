#include "image.h"

#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "std/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "std/stb_image_write.h"

#include "rgbe/rgbe.h" 

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

float* loadFloatPixels(const std::string& _path, int *_width, int *_height, bool _vFlip) {
    FILE* file = fopen(_path.c_str(), "rb");
    RGBE_ReadHeader(file, _width, _height, NULL);

    float* pixels = new float[(*_width) * (*_height) * 3];
    RGBE_ReadPixels_RLE(file, pixels, *_width, *_height);
    
    if (_vFlip) {
        flipPixelsVertically<float>(pixels, (*_width), (*_height), 3);
    }

    fclose(file);
    return pixels;
}

unsigned char* loadPixels(const std::string& _path, int *_width, int *_height, Channels _channels, bool _vFlip) {
    stbi_set_flip_vertically_on_load(_vFlip);
    int comp;
    unsigned char* pixels = stbi_load(_path.c_str(), _width, _height, &comp, (_channels == RGB)? STBI_rgb : STBI_rgb_alpha);
    return pixels;
}

bool savePixels(const std::string& _path, unsigned char* _pixels, int _width, int _height) {

    // Flip the image on Y
    int depth = 4;
    flipPixelsVertically<unsigned char>(_pixels, _width, _height, depth);

    if (0 == stbi_write_png(_path.c_str(), _width, _height, 4, _pixels, _width * 4)) {
        std::cout << "can't create file " << _path << std::endl;
    }

    return true;
}
