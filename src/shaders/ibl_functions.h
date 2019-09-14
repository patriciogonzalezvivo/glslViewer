#pragma once

#include <string>

const std::string ibl_functions = "\n\
\n\
vec3 tonemap(const vec3 x) { return x / (x + 0.155) * 1.019; }\n\
float myPow(float a, float b) { return a / ((1. - b) * a + b); }\n\
\n\
vec3 sphericalHarmonics(vec3 _normal) {\n\
    return max( 0.282095 * u_SH[0]\n\
        + -0.488603 * u_SH[1] * (_normal.y)\n\
        +  0.488603 * u_SH[2] * (_normal.z)\n\
        + -0.488603 * u_SH[3] * (_normal.x)\n\
        , 0.0);\n\
}\n\
\n\
vec3 fakeCubeMap(vec3 _normal) {\n\
    vec3 rAbs = abs(_normal);\n\
    return vec3(1.0) * myPow(max(max(rAbs.x, rAbs.y), rAbs.z) + 0.005, getMaterialShininess());\n\
}\n\
\n\
const float numMips = 6.0;\n\
vec3 prefilterEnvMap(vec3 _normal, float _roughness) {\n\
#if defined(CUBE_MAP)\n\
    vec4 color = mix(   textureCube( CUBE_MAP, _normal, (_roughness * numMips) ), \n\
                        textureCube( CUBE_MAP, _normal, min((_roughness * numMips) + 1.0, numMips)), \n\
                        fract(_roughness * numMips) );\n\
    return tonemap(color.rgb);\n\
#else\n\
    return fakeCubeMap(_normal);\n\
#endif\n\
}\n\
\n\
vec3 envBRDFApprox( vec3 _specColor, float _roughness, float _NoV ) {\n\
    vec4 c0 = vec4( -1, -0.0275, -0.572, 0.022 );\n\
    vec4 c1 = vec4( 1, 0.0425, 1.04, -0.04 );\n\
    vec4 r = _roughness * c0 + c1;\n\
    float a004 = min( r.x * r.x, exp2( -9.28 * _NoV ) ) * r.x + r.y;\n\
    vec2 AB = vec2( -1.04, 1.04 ) * a004 + r.zw;\n\
    return _specColor * AB.x + AB.y;\n\
}\n\
\n\
vec3 approximateSpecularIBL(vec3 _specColor, vec3 _R, float _NoV, float _roughness) {\n\
    vec3 PrefilteredColor = prefilterEnvMap(_R, _roughness);\n\
    return _specColor * PrefilteredColor * envBRDFApprox(_specColor, _roughness, _NoV);\n\
}\n\
\n\
vec3 fresnel(vec3 _R, float _NoV, float _roughness, float _f0) {\n\
    float base = 1.0 - clamp(_NoV, 0.0, 0.99);\n\
    float exponential = pow(base, 5.0);\n\
    float frsnl = _f0 + (1.0 - _f0) * exponential;\n\
    vec3 reflectColor = vec3(0.0);\n\
\n\
    #if defined(CUBE_MAP)\n\
    reflectColor = prefilterEnvMap(_R, _roughness);\n\
    #elif defined(SH_ARRAY)\n\
    reflectColor = sphericalHarmonics(_R);\n\
    #else\n\
    reflectColor = fakeCubeMap(_R);\n\
    #endif\n\
\n\
    return reflectColor * frsnl;\n\
}\n\
\n";