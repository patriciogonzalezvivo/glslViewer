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


// Usefull code from Filament
// https://github.com/google/filament/blob/master/tools/cmgen/src/CubemapSH.cpp


/*
 * returns n! / d!
 */
static double factorial(size_t n, size_t d = 1) {
   d = std::max(size_t(1), d);
   n = std::max(size_t(1), n);
   double r = 1.0;
   if (n == d) {
       // intentionally left blank
   } else if (n > d) {
       for ( ; n>d ; n--) {
           r *= n;
       }
   } else {
       for ( ; d>n ; d--) {
           r *= d;
       }
       r = 1.0 / r;
   }
   return r;
}

// < cos(theta) > SH coefficients pre-multiplied by 1 / K(0,l)
double computeTruncatedCosSh(size_t l) {
    if (l == 0) {
        return M_PI;
    } else if (l == 1) {
        return 2 * M_PI / 3;
    } else if (l & 1) {
        return 0;
    }
    const size_t l_2 = l / 2;
    double A0 = ((l_2 & 1) ? 1.0 : -1.0) / ((l + 2) * (l - 1));
    double A1 = factorial(l, l_2) / (factorial(l_2) * (1 << l));
    return 2 * M_PI * A0 * A1;
}


// /*
//  * This computes the 3-bands SH coefficients of the Cubemap convoluted by the
//  * truncated cos(theta) (i.e.: saturate(s.z)), pre-scaled by the reconstruction
//  * factors.
//  */
// std::unique_ptr<glm::vec3[]> computeIrradianceSH3Bands(const Cubemap& cm) {

//     const size_t numCoefs = 9;

//     std::unique_ptr<glm::vec3[]> SH(new glm::vec3[numCoefs]{});
//     std::unique_ptr<double[]> A(new double[numCoefs]{});

//     const double c0 = computeTruncatedCosSh(0);
//     const double c1 = computeTruncatedCosSh(1);
//     const double c2 = computeTruncatedCosSh(2);
//     A[0] = sq(M_2_SQRTPI / 4)       * c0;
//     A[1] = sq(M_2_SQRTPI / 4) * 3   * c1;
//     A[2] = sq(M_2_SQRTPI / 4) * 3   * c1;
//     A[3] = sq(M_2_SQRTPI / 4) * 3   * c1;
//     A[4] = sq(M_2_SQRTPI / 4) * 15  * c2;
//     A[5] = sq(M_2_SQRTPI / 4) * 15  * c2;
//     A[6] = sq(M_2_SQRTPI / 8) * 5   * c2;
//     A[7] = sq(M_2_SQRTPI / 4) * 15  * c2;
//     A[8] = sq(M_2_SQRTPI / 8) * 15  * c2;

//     struct State {
//         glm::vec3 SH[9] = { };
//     };

//     CubemapUtils::process<State>(cm,
//             [&](State& state, size_t y, Cubemap::Face f, Cubemap::Texel const* data, size_t dim) {
//         for (size_t x=0 ; x<dim ; ++x, ++data) {

//             glm::vec3 s(cm.getDirectionFor(f, x, y));

//             // sample a color
//             glm::vec3 color(Cubemap::sampleAt(data));

//             // take solid angle into account
//             color *= CubemapUtils::solidAngle(dim, x, y);

//             state.SH[0] += color * A[0];
//             state.SH[1] += color * A[1] * s.y;
//             state.SH[2] += color * A[2] * s.z;
//             state.SH[3] += color * A[3] * s.x;
//             state.SH[4] += color * A[4] * s.y*s.x;
//             state.SH[5] += color * A[5] * s.y*s.z;
//             state.SH[6] += color * A[6] * (3*s.z*s.z - 1);
//             state.SH[7] += color * A[7] * s.z*s.x;
//             state.SH[8] += color * A[8] * (s.x*s.x - s.y*s.y);
//         }
//     },
//     [&](State& state) {
//         for (size_t i=0 ; i<numCoefs ; i++) {
//             SH[i] += state.SH[i];
//         }
//     });

//     return SH;
// }