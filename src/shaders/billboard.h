#pragma once

#include <string>

static const std::string billboard_vert = R"(
#ifdef GL_ES
precision mediump float;
#endif

attribute vec4 a_position;
attribute vec2 a_texcoord;

varying vec4 v_position;
// varying vec4 v_color;
// varying vec3 v_normal;
varying vec2 v_texcoord;

#ifdef LIGHT_SHADOWMAP
uniform mat4    u_lightMatrix;
varying vec4    v_lightCoord;
#endif

void main(void) {
    v_position =  a_position;
    // v_color = vec4(1.0);
    // v_normal = vec3(0.0,0.0,1.0);
    v_texcoord = a_texcoord;
    
#ifdef LIGHT_SHADOWMAP
    v_lightCoord = u_lightMatrix * v_position;
#endif
    
    gl_Position = v_position;
}
)";

static const std::string billboard_vert_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

in      vec4    a_position;
in      vec2    a_texcoord;

out     vec4    v_position;
// out     vec4    v_color;
// out     vec3    v_normal;
out     vec2    v_texcoord;

#ifdef LIGHT_SHADOWMAP
uniform mat4    u_lightMatrix;
out     vec4    v_lightCoord;
#endif

void main(void) {
    v_position = a_position;
    // v_color = vec4(1.0);
    // v_normal = vec3(0.0,0.0,1.0);

    v_texcoord = a_texcoord;
    
#ifdef LIGHT_SHADOWMAP
    v_lightCoord = u_lightMatrix * v_position;
#endif
    
    gl_Position = v_position;
}
)";
