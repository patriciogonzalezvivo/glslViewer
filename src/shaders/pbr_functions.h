#pragma once

#include <string>

const std::string pbr_functions = "\n\
#define PI 3.1415926\n\
#define saturate(x)        clamp(x, 0.0, 1.0)\n\
float   myPow(float a, float b) { return a / ((1. - b) * a + b); }\n\
vec3    myPow(vec3 a, float b) { return a / ((1. - b) * a + b); }\n\
float   phong_diffuse() { return (1.0 / PI); }\n\
\n\
float D_GGX(in float roughness, in float NdH) {\n\
    float m = roughness * roughness;\n\
    float m2 = m * m;\n\
    float d = (NdH * m2 - NdH) * NdH + 1.0;\n\
    return m2 / (PI * d * d);\n\
}\n\
\n\
float G_schlick(in float roughness, in float NdV, in float NdL) {\n\
    float k = roughness * roughness * 0.5;\n\
    float V = NdV * (1.0 - k) + k;\n\
    float L = NdL * (1.0 - k) + k;\n\
    return 0.25 / (V * L);\n\
}\n\
\n\
vec3 cooktorrance_specular(in float NdL, in float NdV, in float NdH, in vec3 specular, in float roughness) {\n\
    float D = D_GGX(roughness, NdH);\n\
    float G = G_schlick(roughness, NdV, NdL);\n\
    float rim = mix(1.0 - roughness * 0.9, 1.0, NdV);\n\
    return (1.0 / rim) * specular * G * D;\n\
}\n\
\n\
vec3 fresnel_factor(in vec3 f0, in float product) {\n\
    return mix(f0, vec3(1.0), pow(1.01 - product, 5.0));\n\
}\n\
\n";
