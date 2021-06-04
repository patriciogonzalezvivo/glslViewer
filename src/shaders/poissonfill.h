#pragma once

#include <string>

const std::string poissonfill_frag = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_tex0;
uniform sampler2D   u_tex1;
uniform vec2        u_pixel;
uniform bool        u_isup;

const vec3 h = vec3(1.0334, 0.6836, 0.1507);
const vec2 g = vec2(0.7753, 0.0312);

float h1(int i) {
    // if (i == -2 || i == 2) return 0.1507;
    // if (i == -1 || i == 1) return 0.6836;
    // return 1.0334;
    return h[ i < 0 ? i * -1 : i ];
}

float G(int i) {
    // if (i == -1 || i == 1) return 0.0312;
    // return 0.7753;
    return g[ i < 0 ? i * -1 : i ];
}

varying vec2 v_texcoord;

void main() {
    vec4 color = vec4(0.0);
    vec2 st = v_texcoord;
    vec2 pixel = u_pixel;
    st += pixel * 0.5;

    if (!u_isup) {
        // IS DOWN
        for (int dy = -2; dy <= 2; dy++) {
            for (int dx = -2; dx <= 2; dx++) {
                vec2 uv = st + vec2(float(dx), float(dy)) * pixel;
                if (uv.x < 0.0 || uv.x >= 1.0 || uv.y < 0.0 || uv.y >= 1.0)
                    continue;
                color += texture2D(u_tex0, uv) * h1(dx) * h1(dy);
            }
        }
    }
    else {
        // IS UP
        const float h2 = 0.0270;
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                vec2 uv = st + vec2(float(dx) - 0.75, float(dy) - 0.75) * pixel;
                color += texture2D(u_tex0, clamp(uv, 0.0, 1.0)) * G(dx) * G(dy);
            }
        }

        for (int dy = -2; dy <= 2; dy++) {
            for (int dx = -2; dx <= 2; dx++) {
                vec2 uv = st + vec2(float(dx) - 0.75, float(dy) - 0.75) * pixel;
                color += texture2D(u_tex1, clamp(uv, 0.0, 1.0)) * h2 * h1(dx) * h1(dy);
            }
        }
    }

    gl_FragColor = (color.a == 0.0)? color : vec4(color.rgb/color.a, 1.0);
}
)";