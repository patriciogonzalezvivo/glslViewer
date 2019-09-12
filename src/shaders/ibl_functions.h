#pragma once

#include <string>

const std::string ibl_functions = "\n\
#if defined(SH_ARRAY) || defined(CUBE_MAP)\n\
\n\
uniform samplerCube u_cubeMap;\n\
uniform vec3        u_SH[9];\n\
\n\
vec3 tonemap(const vec3 x) { return x / (x + 0.155) * 1.019; }\n\
\n\
vec3 sphericalHarmonics(const vec3 n) {\n\
    return max(\n\
           0.282095 * u_SH[0]\n\
        + -0.488603 * u_SH[1] * (n.y)\n\
        +  0.488603 * u_SH[2] * (n.z)\n\
        + -0.488603 * u_SH[3] * (n.x)\n\
        , 0.0);\n\
}\n\
\n\
vec3 prefilterEnvMap(samplerCube _cube, vec3 _R, float _roughness) {\n\
    float numMips = 6.0;\n\
    vec4 color = mix(   textureCube( _cube, _R, (_roughness * numMips) ), \n\
                        textureCube( _cube, _R, min((_roughness * numMips) + 1.0, numMips)), \n\
                        fract(_roughness * numMips) );\n\
    return tonemap(color.rgb);\n\
}\n\
\n\
vec3 envBRDFApprox( vec3 _specColor, float _roughness, float _NoE ) {\n\
    vec4 c0 = vec4( -1, -0.0275, -0.572, 0.022 );\n\
    vec4 c1 = vec4( 1, 0.0425, 1.04, -0.04 );\n\
    vec4 r = _roughness * c0 + c1;\n\
    float a004 = min( r.x * r.x, exp2( -9.28 * _NoE ) ) * r.x + r.y;\n\
    vec2 AB = vec2( -1.04, 1.04 ) * a004 + r.zw;\n\
    return _specColor * AB.x + AB.y;\n\
}\n\
\n\
vec3 approximateSpecularIBL(samplerCube _cube, vec3 _specColor, vec3 _R, float _NoE, float _roughness) {\n\
    vec3 PrefilteredColor = prefilterEnvMap(_cube, _R, _roughness);\n\
    return _specColor * PrefilteredColor * envBRDFApprox(_specColor, _roughness, _NoE);\n\
}\n\
#endif\n\
\n";