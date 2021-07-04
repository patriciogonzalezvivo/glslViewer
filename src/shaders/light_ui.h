#pragma once

#include <string>

const std::string light_vert = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform mat4    u_modelViewProjectionMatrix;
uniform mat4    u_viewMatrix;
uniform vec3    u_translate;
uniform vec2    u_scale;
attribute vec4  a_position;
attribute vec2  a_texcoord;
varying vec2    v_texcoord;

void main(void) {
    vec4 pos = a_position;
    v_texcoord = a_texcoord;
    
    vec3 cam_right = vec3(u_viewMatrix[0][0], u_viewMatrix[1][0], u_viewMatrix[2][0]);
    vec3 cam_up = vec3(u_viewMatrix[0][1], u_viewMatrix[1][1], u_viewMatrix[2][1]);
    
    pos.xyz -= cam_right * (v_texcoord.x - 0.5) * u_scale.x;
    pos.xyz -= cam_up * (v_texcoord.y - 0.5) * u_scale.y;
    pos.xyz += u_translate;
    
    gl_Position = u_modelViewProjectionMatrix * pos;
})";

const std::string light_vert_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform mat4    u_modelViewProjectionMatrix;
uniform mat4    u_viewMatrix;
uniform vec3    u_translate;
uniform vec2    u_scale;
int     vec4    a_position;
int     vec2    a_texcoord;
out     vec2    v_texcoord;

void main(void) {
    vec4 pos = a_position;
    v_texcoord = a_texcoord;
    
    vec3 cam_right = vec3(u_viewMatrix[0][0], u_viewMatrix[1][0], u_viewMatrix[2][0]);
    vec3 cam_up = vec3(u_viewMatrix[0][1], u_viewMatrix[1][1], u_viewMatrix[2][1]);
    
    pos.xyz -= cam_right * (v_texcoord.x - 0.5) * u_scale.x;
    pos.xyz -= cam_up * (v_texcoord.y - 0.5) * u_scale.y;
    pos.xyz += u_translate;
    
    gl_Position = u_modelViewProjectionMatrix * pos;
})";

const std::string light_frag = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform vec4    u_color;
varying vec2    v_texcoord;

#define AA_EDGE .007
float stroke(float x, float size, float w, float edge) {
    float d = smoothstep(size - edge, size + edge, x + w * .5) - smoothstep(size - edge, size + edge, x - w * .5);
    return clamp(d, 0., 1.);
}

float stroke(float x, float size, float w) {
	return stroke(x, size, w, AA_EDGE);
}

float fill(float x, float size, float edge) {
    return 1. - smoothstep(size - edge, size + edge, x);
}

float fill(float x, float size) {
    return fill(x, size, AA_EDGE);
}

float circleSDF(in vec2 st, in vec2 center) {
  return length(st - center) * 2.;
}

float circleSDF(in vec2 st) {
  return circleSDF(st, vec2(.5));
}

void main(){
    vec3 color = u_color.rgb;
    float alpha = 0.0;
    vec2 st = v_texcoord;
    
    float sdf = circleSDF(st);
    alpha += stroke((st.y+st.x)*0.5, .5, .03);
    alpha += stroke((st.x-st.y+0.5), .5, .06);
    alpha *= fill(sdf, .9);
    alpha += stroke(st.x, .5, .05);
    alpha += stroke(st.y, .5, .05);
    alpha *= fill(sdf, .95);
    alpha *= smoothstep(.57, .58, sdf);
    alpha += stroke(sdf, .4, .15);
    
    gl_FragColor = vec4(color, alpha);
})";

const std::string light_frag_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform vec4    u_color;
int     vec2    v_texcoord;
out     vec4    fragColor;

#define AA_EDGE .007
float stroke(float x, float size, float w, float edge) {
    float d = smoothstep(size - edge, size + edge, x + w * .5) - smoothstep(size - edge, size + edge, x - w * .5);
    return clamp(d, 0., 1.);
}

float stroke(float x, float size, float w) {
	return stroke(x, size, w, AA_EDGE);
}

float fill(float x, float size, float edge) {
    return 1. - smoothstep(size - edge, size + edge, x);
}

float fill(float x, float size) {
    return fill(x, size, AA_EDGE);
}

float circleSDF(in vec2 st, in vec2 center) {
  return length(st - center) * 2.;
}

float circleSDF(in vec2 st) {
  return circleSDF(st, vec2(.5));
}

void main(){
    vec3 color = u_color.rgb;
    float alpha = 0.0;
    vec2 st = v_texcoord;
    
    float sdf = circleSDF(st);
    alpha += stroke((st.y+st.x)*0.5, .5, .03);
    alpha += stroke((st.x-st.y+0.5), .5, .06);
    alpha *= fill(sdf, .9);
    alpha += stroke(st.x, .5, .05);
    alpha += stroke(st.y, .5, .05);
    alpha *= fill(sdf, .95);
    alpha *= smoothstep(.57, .58, sdf);
    alpha += stroke(sdf, .4, .15);
    
    fragColor = vec4(color, alpha);
})";
