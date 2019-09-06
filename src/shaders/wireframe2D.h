#pragma once

#include <string>

const std::string wireframe2D_vert = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
uniform mat4 u_modelViewProjectionMatrix;\n\
uniform vec2 u_translate;\n\
uniform vec2 u_scale;\n\
attribute vec4 a_position;\n\
\n\
void main(void) {\n\
    vec4 position = a_position;\n\
    position.xy *= u_scale;\n\
    position.xy += u_translate;\n\
    gl_Position = u_modelViewProjectionMatrix * position;\n\
}\n";

const std::string wireframe2D_frag = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
uniform vec4 u_color;\n\
\n\
void main(void) { \n\
    gl_FragColor = u_color;\n\
}\n";
