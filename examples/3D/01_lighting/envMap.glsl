#include "tonemap.glsl"

#ifndef FNC_ENVMAP
#define FNC_ENVMAP

uniform samplerCube u_cubeMap;
const float         numMips = 6.0;

vec3 envMap(vec3 _normal, float _roughness) {
    vec4 color = mix(   textureCube( u_cubeMap, _normal, (_roughness * numMips) ), 
                        textureCube( u_cubeMap, _normal, min((_roughness * numMips) + 1.0, numMips)), 
                        fract(_roughness * numMips) );
    return tonemap(color.rgb);
}

#endif