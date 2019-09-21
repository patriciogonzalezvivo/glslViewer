#pragma once

#include <string>

static const std::string billboard_vert = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
attribute vec4 a_position;\n\
attribute vec2 a_texcoord;\n\
\n\
varying vec4 v_position;\n\
// varying vec4 v_color;\n\
// varying vec3 v_normal;\n\
varying vec2 v_texcoord;\n\
\n\
#ifdef LIGHT_SHADOWMAP\n\
uniform mat4    u_lightMatrix;\n\
varying vec4    v_lightCoord;\n\
#endif\n\
\n\
void main(void) {\n\
    v_position =  a_position;\n\
    // v_color = vec4(1.0);\n\
    // v_normal = vec3(0.0,0.0,1.0);\n\
    v_texcoord = a_texcoord;\n\
    \n\
#ifdef LIGHT_SHADOWMAP\n\
    v_lightCoord = u_lightMatrix * v_position;\n\
#endif\n\
    \n\
    gl_Position = v_position;\n\
}";
