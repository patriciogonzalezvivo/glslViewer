#pragma once

#include <string>


// DEFAULT SHADERS
// -----------------------------------------------------
const std::string default_vert = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
uniform mat4 u_modelViewProjectionMatrix;\n\
\n\
attribute vec4  a_position;\n\
varying vec4    v_position;\n\
\n\
#ifdef MODEL_VERTEX_COLOR\n\
attribute vec4  a_color;\n\
varying vec4    v_color;\n\
#endif\n\
\n\
#ifdef MODEL_VERTEX_NORMAL\n\
attribute vec3  a_normal;\n\
varying vec3    v_normal;\n\
#endif\n\
\n\
#ifdef MODEL_VERTEX_TEXCOORD\n\
attribute vec2  a_texcoord;\n\
varying vec2    v_texcoord;\n\
#endif\n\
\n\
void main(void) {\n\
    \n\
    v_position = a_position;\n\
    \n\
#ifdef MODEL_VERTEX_COLOR\n\
    v_color = a_color;\n\
#endif\n\
    \n\
#ifdef MODEL_VERTEX_NORMAL\n\
    v_normal = a_normal;\n\
#endif\n\
    \n\
#ifdef MODEL_VERTEX_TEXCOORD\n\
    v_texcoord = a_texcoord;\n\
#endif\n\
    \n\
    gl_Position = u_modelViewProjectionMatrix * v_position;\n\
}\n\
";

const std::string default_frag = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
uniform vec2    u_resolution;\n\
uniform vec2    u_mouse;\n\
uniform float   u_time;\n\
\n\
void main(void) {\n\
    vec3 color = vec3(1.0);\n\
    vec2 st = gl_FragCoord.xy/u_resolution.xy;\n\
    st.x *= u_resolution.x/u_resolution.y;\n\
    \n\
    color = vec3(st.x,st.y,abs(sin(u_time)));\n\
    \n\
    gl_FragColor = vec4(color, 1.0);\n\
}\n";
