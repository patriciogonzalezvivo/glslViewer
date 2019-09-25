#pragma once

#include <string>


// DEFAULT SHADERS
// -----------------------------------------------------
const std::string default_vert = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform mat4 u_modelViewProjectionMatrix;

attribute vec4  a_position;
varying vec4    v_position;

#ifdef MODEL_VERTEX_COLOR
attribute vec4  a_color;
varying vec4    v_color;
#endif

#ifdef MODEL_VERTEX_NORMAL
attribute vec3  a_normal;
varying vec3    v_normal;
#endif

#ifdef MODEL_VERTEX_TEXCOORD
attribute vec2  a_texcoord;
varying vec2    v_texcoord;
#endif

void main(void) {
    
    v_position = a_position;
    
#ifdef MODEL_VERTEX_COLOR
    v_color = a_color;
#endif
    
#ifdef MODEL_VERTEX_NORMAL
    v_normal = a_normal;
#endif
    
#ifdef MODEL_VERTEX_TEXCOORD
    v_texcoord = a_texcoord;
#endif
    
    gl_Position = u_modelViewProjectionMatrix * v_position;
})";

const std::string default_frag = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform vec2    u_resolution;
uniform vec2    u_mouse;
uniform float   u_time;

void main(void) {
    vec3 color = vec3(1.0);
    vec2 st = gl_FragCoord.xy/u_resolution.xy;
    st.x *= u_resolution.x/u_resolution.y;
    
    color = vec3(st.x,st.y,abs(sin(u_time)));
    
    gl_FragColor = vec4(color, 1.0);
})";
