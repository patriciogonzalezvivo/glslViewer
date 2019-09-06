#pragma once

#include <string>

static const std::string error_vert = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
uniform mat4 u_modelViewProjectionMatrix;\n\
\n\
attribute vec4  a_position;\n\
varying vec4    v_position;\n\
\n\
void main(void) {\n\
    v_position = a_position;\n\
    gl_Position = u_modelViewProjectionMatrix * v_position;\n\
}\n\
";

static const std::string error_frag = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
void main(void) {\n\
    vec3 color = vec3(1.0, 0.0, 1.0);\n\
    gl_FragColor = vec4(color, 1.0);\n\
}\n";
