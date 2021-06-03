#pragma once

#include <string>

const std::string poissonfill_frag = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_tex0;
uniform sampler2D   u_tex1;
// uniform vec2        u_resolution;
uniform vec2        u_pixel;
uniform bool        u_isup;

float h1(float i) {
    if (i == 0.0 || i == 4.0)
        return 0.1507;
    
    if (i == 1.0 || i == 3.0)
        return 0.6836;

    return 1.0334;
}

float G(float i) {
    if (i == 0.0 || i == 2.0)
        return 0.0312;

    return 0.7753;
}

varying vec2 v_texcoord;

void main() {
    vec2 st = v_texcoord;
    vec2 pixel = u_pixel;

    if (!u_isup) {
        st *= 2.0;
        vec4 acc = vec4(0.0);
        for (float dy = -2.0; dy <= 2.0; dy++) {
            for (float dx = -2.0; dx <= 2.0; dx++) {
                vec2 uv = st + vec2(dx, dy) * pixel;
                // if (uv.x < 0.0 || uv.x >= 1.0 || uv.y < 0.0 || uv.y >= 1.0)
                //     continue;
                acc += texture2D(u_tex0, uv) * h1(dx + 2.0) * h1(dy + 2.0);
            }
        }

        if (acc.a == 0.0) 
            gl_FragColor = acc;
        else
            gl_FragColor = vec4(acc.rgb/acc.a,1.0);
    }
    else {
        float h2 = 0.0270;
        vec4 highp acc = vec4(0.0);
        for (float dy = -1.0; dy <= 1.0; dy++) {
            for (float dx = -1.0; dx <= 1.0; dx++) {
                vec2 uv = st + vec2(dx, dy) * pixel;
                // if (uv.x < 0.0 || uv.x >= 1.0 || uv.y < 0.0 || uv.y >= 1.0)
                //     continue;
                acc += texture2D(u_tex0, uv) * G(dx + 1.0) * G(dy + 1.0);
            }
        }

        for (float dy = -2.0; dy <= 2.0; dy++) {
            for (float dx = -2.0; dx <= 2.0; dx++) {
                vec2 uv = st + vec2(dx, dy) * pixel;
                // if (uv.x < 0.0 || uv.x >= 1.0 || uv.y < 0.0 || uv.y >= 1.0)
                //     continue;
                uv *= 0.5;
                acc += texture2D(u_tex1, uv) * h2 * h1(dx + 2.0) * h1(dy + 2.0);
            }
        }

        if (acc.a == 0.0)
            gl_FragColor = acc;
        else
            gl_FragColor = vec4(acc.rgb/acc.a,1.0);
    }
}
)";