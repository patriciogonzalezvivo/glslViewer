#pragma once

#include <string>

std::string wireframe3D_vert = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
uniform mat4    u_modelViewProjectionMatrix;\n\
attribute vec4  a_position;\n\
\n\
void main(void) {\n\
    gl_Position = u_modelViewProjectionMatrix * a_position;\n\
}";

std::string wireframe3D_frag = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
uniform vec4 u_color;\n\
\n\
void main(void) {\n\
    gl_FragColor = u_color;\n\
}\n";
