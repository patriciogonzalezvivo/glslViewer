#pragma once

#include <string>

const std::string shadows_functions = "\n\
#if defined(SHADOW_MAP) && defined(SHADOW_MAP_SIZE)\n\
uniform sampler2D u_ligthShadowMap;\n\
varying vec4    v_lightcoord;\n\
\n\
float textureShadow(const sampler2D depths, vec2 uv, float compare){\n\
    return step(compare, texture2D(depths, uv).r );\n\
}\n\
\n\
float textureShadowLerp(sampler2D depths, vec2 size, vec2 uv, float compare){\n\
    vec2 texelSize = vec2(1.0)/size;\n\
    vec2 f = fract(uv*size+0.5);\n\
    vec2 centroidUV = floor(uv*size+0.5)/size;\n\
    float lb = textureShadow(depths, centroidUV+texelSize*vec2(0.0, 0.0), compare);\n\
    float lt = textureShadow(depths, centroidUV+texelSize*vec2(0.0, 1.0), compare);\n\
    float rb = textureShadow(depths, centroidUV+texelSize*vec2(1.0, 0.0), compare);\n\
    float rt = textureShadow(depths, centroidUV+texelSize*vec2(1.0, 1.0), compare);\n\
    float a = mix(lb, lt, f.y);\n\
    float b = mix(rb, rt, f.y);\n\
    float c = mix(a, b, f.x);\n\
    return c;\n\
}\n\
\n\
float textureShadowPCF(sampler2D depths, vec2 size, vec2 uv, float compare) {\n\
    float result = 0.0;\n\
    for(int x=-2; x<=2; x++){\n\
        for(int y=-2; y<=2; y++){\n\
            vec2 off = vec2(x,y)/size;\n\
            // result += sampleShadow(depths, uv+off, compare);\n\
            result += textureShadowLerp(depths, size, uv+off, compare);\n\
        }\n\
    }\n\
    return result/25.0;\n\
}\n\
#endif\n\
\n";