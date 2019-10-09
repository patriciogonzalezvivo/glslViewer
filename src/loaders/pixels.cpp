#include "pixels.h"

#include <stdio.h>
#include <iostream>
#include <math.h>
// #include <assert.h>
#include <vector>
// #include <algorithm>

#include "rgbe/rgbe.h"

float* loadPixelsHDR(const std::string& _path, int *_width, int *_height, bool _vFlip) {
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