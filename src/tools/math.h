#pragma once

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif 

#ifndef M_2_SQRTPI
#define M_2_SQRTPI 1.12837916709551257390
#endif

#ifndef M_MIN
#define M_MIN(_a, _b) ((_a)<(_b)?(_a):(_b))
#endif

static float angleBetween(float thetav, float phiv, float theta, float phi) {
    float cosGamma = sinf(thetav) * sinf(theta) * cosf(phi - phiv) + cosf(thetav) * cosf(theta);
    return acosf(cosGamma);
}


float clamp ( float value , float min , float max ) {
    if (value < min)
        return min;
    if (value > max)
        return max;

    return value;
}