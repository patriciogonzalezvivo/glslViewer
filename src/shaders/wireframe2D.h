#pragma once

#include <string>

const std::string wireframe2D_vert = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform mat4 u_modelViewProjectionMatrix;
uniform vec2 u_translate;
uniform vec2 u_scale;
attribute vec4 a_position;

void main(void) {
    vec4 position = a_position;
    position.xy *= u_scale;
    position.xy += u_translate;
    gl_Position = u_modelViewProjectionMatrix * position;
})";

const std::string wireframe2D_frag = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform vec4 u_color;

void main(void) {
    gl_FragColor = u_color;
})";


const std::string wireframe2D_vert_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform mat4 u_modelViewProjectionMatrix;
uniform vec2 u_translate;
uniform vec2 u_scale;
in      vec4 a_position;

void main(void) {
    vec4 position = a_position;
    position.xy *= u_scale;
    position.xy += u_translate;
    gl_Position = u_modelViewProjectionMatrix * position;
})";

const std::string wireframe2D_frag_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform vec4 u_color;
out     vec4 fragColor;

void main(void) {
    fragColor = u_color;
})";
