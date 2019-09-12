#pragma once

#include <string>

const std::string pbr_functions = "\n\
#define PI 3.1415926\n\
#define saturate(x)        clamp(x, 0.0, 1.0)\n\
float   myPow(float a, float b) { return a / ((1. - b) * a + b); }\n\
vec3    myPow(vec3 a, float b) { return a / ((1. - b) * a + b); }\n\
float   phong_diffuse() { return (1.0 / PI); }\n\
\n";
