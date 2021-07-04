#pragma once

#include <string>

const std::string dynamic_billboard_vert = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform mat4 u_modelViewProjectionMatrix;
uniform vec2 u_translate;
uniform vec2 u_scale;
attribute vec4 a_position;
attribute vec2 a_texcoord;
varying vec2 v_texcoord;

void main(void) {
    vec4 position = a_position;
    position.xy *= u_scale;
    position.xy += u_translate;
    v_texcoord = a_texcoord;
    gl_Position = u_modelViewProjectionMatrix * position;
})";

const std::string dynamic_billboard_frag = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D u_tex0;
uniform vec4 u_color;
uniform float u_depth;
uniform float u_cameraNearClip;
uniform float u_cameraFarClip;
uniform float u_cameraDistance;

varying vec2 v_texcoord;

float linearizeDepth(float zoverw) {
	return (2.0 * u_cameraNearClip) / (u_cameraFarClip + u_cameraNearClip - zoverw * (u_cameraFarClip - u_cameraNearClip));
}

vec3 heatmap(float v) {
    vec3 r = v * 2.1 - vec3(1.8, 1.14, 0.3);
    return 1.0 - r * r;
}

void main(void) { 
    vec4 color = u_color;
    color += texture2D(u_tex0, v_texcoord);
    
    if (u_depth > 0.0) {
        color.r = linearizeDepth(color.r) * u_cameraFarClip;
        color.rgb = heatmap(1.0 - (color.r - u_cameraDistance) * 0.01);
    }
    
    gl_FragColor = color;
}
)";

const std::string dynamic_billboard_vert_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform mat4 u_modelViewProjectionMatrix;
uniform vec2 u_translate;
uniform vec2 u_scale;
in      vec4 a_position;
in      vec2 a_texcoord;
out     vec2 v_texcoord;

void main(void) {
    vec4 position = a_position;
    position.xy *= u_scale;
    position.xy += u_translate;
    v_texcoord = a_texcoord;
    gl_Position = u_modelViewProjectionMatrix * position;
})";


const std::string dynamic_billboard_frag_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_tex0;
uniform vec4        u_color;
uniform float       u_depth;
uniform float       u_cameraNearClip;
uniform float       u_cameraFarClip;
uniform float       u_cameraDistance;

in      vec2        v_texcoord;
out     vec4        fragColor;

float linearizeDepth(float zoverw) {
	return (2.0 * u_cameraNearClip) / (u_cameraFarClip + u_cameraNearClip - zoverw * (u_cameraFarClip - u_cameraNearClip));
}

vec3 heatmap(float v) {
    vec3 r = v * 2.1 - vec3(1.8, 1.14, 0.3);
    return 1.0 - r * r;
}

void main(void) { 
    vec4 color = u_color;
    color += texture(u_tex0, v_texcoord);
    
    if (u_depth > 0.0) {
        color.r = linearizeDepth(color.r) * u_cameraFarClip;
        color.rgb = heatmap(1.0 - (color.r - u_cameraDistance) * 0.01);
    }
    
    fragColor = color;
}
)";
