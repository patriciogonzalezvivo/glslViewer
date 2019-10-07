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

void rescalePixels(const unsigned char* _srcData, int _srcWidth, int _srcHeight, int _srcChannels, int _dstWidth, int _dstHeight, unsigned char* _dstData) {
    std::cout << _srcWidth << "x" << _srcHeight << " to " << _dstWidth << "x" << _dstHeight << std::endl;
    int x, y, i, c;
    int x_src, y_src, i_src;
    for (y = 0; y < _dstHeight; y++) {
        for (x = 0; x < _dstWidth; x++) {
            i = y * _dstWidth + x;
            y_src = ((float)(y)/(float)(_dstHeight-1)) * (_srcHeight-1);
            x_src = ((float)(x)/(float)(_dstWidth-1)) * (_srcWidth-1);
            i_src = y_src * _srcWidth + x_src;

            for (c = 0; c < _srcChannels; c++) {
                _dstData[i * _srcChannels + c] = _srcData[i_src * _srcChannels + c];
            }
        }
    }
}
