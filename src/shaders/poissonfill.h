#pragma once

#include <string>

// Reference:
// - Convolution Pyramids, Farbman et al., 2011 (https://www.cse.huji.ac.il/labs/cglab/projects/convpyr/data/convpyr-small.pdf)
// - Rendu (https://github.com/kosua20/Rendu)
// - ofxPoissonFill (https://github.com/LingDong-/ofxPoissonFill)
//

const std::string poissonfill_frag = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_convolutionPyramidTex0;
uniform sampler2D   u_convolutionPyramidTex1;
uniform bool        u_convolutionPyramidUpscaling;

uniform vec2        u_resolution;
uniform vec2        u_pixel;

const vec3 h1 = vec3(1.0334, 0.6836, 0.1507);
const float h2 = 0.0270;
const vec2 g = vec2(0.7753, 0.0312);

#define saturate(x) clamp(x, 0.0, 1.0)
#define absi(x)     ( (x < 0)? x * -1 : x )
// #define NON_CONST_ARRAY_INDEX

float H1(int i) {
#ifndef NON_CONST_ARRAY_INDEX
    if (i == -2 || i == 2) return h1[2];
    if (i == -1 || i == 1) return h1[1];
    return h1[0];
#else
    return h1[ absi(i) ];
#endif
}

float G(int i) {
#ifndef NON_CONST_ARRAY_INDEX
    if (i == -1 || i == 1) return g[1];
    return g[0];
#else
    return g[ absi(i) ];
#endif
}

varying vec2 v_texcoord;

void main() {
    vec4 color = vec4(0.0);
    vec2 pixel = u_pixel;
    vec2 st = v_texcoord;

    if (!u_convolutionPyramidUpscaling) {
        // DOWNSCALE
        //
        //  u_convolutionPyramidTex0: previous pass (bigger)
        //
        for (int dy = -2; dy <= 2; dy++) {
            for (int dx = -2; dx <= 2; dx++) {
                vec2 uv = st + vec2(float(dx), float(dy)) * pixel * 0.5;
                if (uv.x <= 0.0 || uv.x >= 1.0 || uv.y <= 0.0 || uv.y >= 1.0)
                    continue;
                color += texture2D(u_convolutionPyramidTex0, saturate(uv)) * H1(dx) * H1(dy);
            }
        }
    }
    else {
        // UPSCALE
        //
        //  u_convolutionPyramidTex0: unfiltered counterpart (same size)
        //  u_convolutionPyramidTex1: previous pass (smaller)
        //
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                vec2 uv = st + vec2(float(dx), float(dy)) * pixel;
                color += texture2D(u_convolutionPyramidTex0, saturate(uv)) * G(dx) * G(dy);
            }
        }

        for (int dy = -2; dy <= 2; dy++) {
            for (int dx = -2; dx <= 2; dx++) {
                vec2 uv = st + vec2(float(dx), float(dy)) * pixel * 2.0;
                if (uv.x <= 0.0 || uv.x >= 1.0 || uv.y <= 0.0 || uv.y >= 1.0)
                    continue;
                color += texture2D(u_convolutionPyramidTex1, saturate(uv)) * h2 * H1(dx) * H1(dy);
            }
        }
    }

    gl_FragColor = (color.a == 0.0)? color : vec4(color.rgb/color.a, 1.0);
}
)";


const std::string poissonfill_frag_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_convolutionPyramidTex0;
uniform sampler2D   u_convolutionPyramidTex1;
uniform bool        u_convolutionPyramidUpscaling;

uniform vec2        u_resolution;
uniform vec2        u_pixel;

in      vec2        v_texcoord;
out     vec4        fragColor;

const vec3 h1 = vec3(1.0334, 0.6836, 0.1507);
const float h2 = 0.0270;
const vec2 g = vec2(0.7753, 0.0312);

#define saturate(x) clamp(x, 0.0, 1.0)
#define absi(x)     ( (x < 0)? x * -1 : x )
// #define NON_CONST_ARRAY_INDEX

float H1(int i) {
#ifndef NON_CONST_ARRAY_INDEX
    if (i == -2 || i == 2) return h1[2];
    if (i == -1 || i == 1) return h1[1];
    return h1[0];
#else
    return h1[ absi(i) ];
#endif
}

float G(int i) {
#ifndef NON_CONST_ARRAY_INDEX
    if (i == -1 || i == 1) return g[1];
    return g[0];
#else
    return g[ absi(i) ];
#endif
}

void main() {
    vec4 color = vec4(0.0);
    vec2 pixel = u_pixel;
    vec2 st = v_texcoord;

    if (!u_convolutionPyramidUpscaling) {
        // DOWNSCALE
        //
        //  u_convolutionPyramidTex0: previous pass (bigger)
        //
        for (int dy = -2; dy <= 2; dy++) {
            for (int dx = -2; dx <= 2; dx++) {
                vec2 uv = st + vec2(float(dx), float(dy)) * pixel * 0.5;
                if (uv.x <= 0.0 || uv.x >= 1.0 || uv.y <= 0.0 || uv.y >= 1.0)
                    continue;
                color += texture(u_convolutionPyramidTex0, saturate(uv)) * H1(dx) * H1(dy);
            }
        }
    }
    else {
        // UPSCALE
        //
        //  u_convolutionPyramidTex0: unfiltered counterpart (same size)
        //  u_convolutionPyramidTex1: previous pass (smaller)
        //
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                vec2 uv = st + vec2(float(dx), float(dy)) * pixel;
                color += texture(u_convolutionPyramidTex0, saturate(uv)) * G(dx) * G(dy);
            }
        }

        for (int dy = -2; dy <= 2; dy++) {
            for (int dx = -2; dx <= 2; dx++) {
                vec2 uv = st + vec2(float(dx), float(dy)) * pixel * 2.0;
                if (uv.x <= 0.0 || uv.x >= 1.0 || uv.y <= 0.0 || uv.y >= 1.0)
                    continue;
                color += texture(u_convolutionPyramidTex1, saturate(uv)) * h2 * H1(dx) * H1(dy);
            }
        }
    }

    fragColor = (color.a == 0.0)? color : vec4(color.rgb/color.a, 1.0);
}
)";