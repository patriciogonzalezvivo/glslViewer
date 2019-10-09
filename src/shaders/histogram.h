#pragma once

#include <string>

const std::string histogram_frag = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D u_sceneHistogram;

varying vec2 v_texcoord;

void main() {
    vec3 color = vec3(0.0);
    vec2 st = v_texcoord;

    vec3 freqs = texture2D(u_sceneHistogram, vec2(st.x, 0.0)).rgb;

    color.r = step(st.y, freqs.r);
    color.g = step(st.y, freqs.g);
    color.b = step(st.y, freqs.b);

    gl_FragColor = vec4(color, 1.0);
}
)";
