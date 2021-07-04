#pragma once

#include <string>

static const std::string error_vert = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform mat4    u_modelViewProjectionMatrix;

attribute vec4  a_position;
varying vec4    v_position;

void main(void) {
    v_position = a_position;
    gl_Position = u_modelViewProjectionMatrix * v_position;
}
)";

static const std::string error_frag = R"(
#ifdef GL_ES
precision mediump float;
#endif

void main(void) {
    gl_FragColor = vec4(1.0, 0.0, 1.0, 1.0);
}
)";

static const std::string error_vert_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform mat4    u_modelViewProjectionMatrix;

in      vec4    a_position;
out     vec4    v_position;

void main(void) {
    v_position = a_position;
    gl_Position = u_modelViewProjectionMatrix * v_position;
}
)";


static const std::string error_frag_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

out     vec4    fragColor;

void main(void) {
    fragColor = vec4(1.0, 0.0, 1.0, 1.0);
}
)";
