#pragma once

#include <string>

std::string billboard_vert = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
attribute vec4 a_position;\n\
attribute vec2 a_texcoord;\n\
\n\
varying vec4 v_position;\n\
varying vec4 v_color;\n\
varying vec3 v_normal;\n\
varying vec2 v_texcoord;\n\
\n\
#ifdef SHADOW_MAP\n\
uniform mat4    u_lightMatrix;\n\
varying vec4    v_lightcoord;\n\
#endif\n\
\n\
void main(void) {\n\
    v_position =  a_position;\n\
    v_color = vec4(1.0);\n\
    v_normal = vec3(0.0,0.0,1.0);\n\
    v_texcoord = a_texcoord;\n\
    \n\
#ifdef SHADOW_MAP\n\
    v_lightcoord = u_lightMatrix * v_position;\n\
#endif\n\
    \n\
    gl_Position = v_position;\n\
}";

std::string dynamic_billboard_vert = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
uniform mat4 u_modelViewProjectionMatrix;\n\
uniform vec2 u_translate;\n\
uniform vec2 u_scale;\n\
attribute vec4 a_position;\n\
attribute vec2 a_texcoord;\n\
varying vec2 v_texcoord;\n\
\n\
void main(void) {\n\
    vec4 position = a_position;\n\
    position.xy *= u_scale;\n\
    position.xy += u_translate;\n\
    v_texcoord = a_texcoord;\n\
    gl_Position = u_modelViewProjectionMatrix * position;\n\
}";

std::string dynamic_billboard_frag = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
uniform sampler2D u_tex0;\n\
uniform vec4 u_color;\n\
uniform float u_depth;\n\
uniform float u_cameraNearClip;\n\
uniform float u_cameraFarClip;\n\
uniform float u_cameraDistance;\n\
\n\
varying vec2 v_texcoord;\n\
\n\
float linearizeDepth(float zoverw) {\n\
	return (2.0 * u_cameraNearClip) / (u_cameraFarClip + u_cameraNearClip - zoverw * (u_cameraFarClip - u_cameraNearClip));\n\
}\n\
\n\
vec3 heatmap(float v) {\n\
    vec3 r = v * 2.1 - vec3(1.8, 1.14, 0.3);\n\
    return 1.0 - r * r;\n\
}\n\
\n\
void main(void) { \n\
    vec4 color = u_color;\n\
    color += texture2D(u_tex0, v_texcoord);\n\
    \n\
    if (u_depth > 0.0) {\n\
        color.r = linearizeDepth(color.r) * u_cameraFarClip;\n\
        color.rgb = heatmap(1.0 - (color.r - u_cameraDistance) * 0.01);\n\
    }\n\
    \n\
    gl_FragColor = color;\n\
}\n";
