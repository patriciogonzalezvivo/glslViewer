#ifdef GL_ES
precision mediump float;
#endif
uniform mat4 u_modelViewProjectionMatrix;
uniform mat4 u_modelMatrix;
uniform mat4 u_viewMatrix;
uniform mat4 u_projectionMatrix;
uniform mat4 u_normalMatrix;

uniform vec4 u_date;
uniform vec3 u_centre3d;
uniform vec3 u_eye3d;
uniform vec3 u_up3d;
uniform vec3 u_view2d;
uniform vec2 u_resolution;
uniform vec2 u_mouse;
uniform float u_time;
uniform float u_delta;

attribute vec4 a_position;
varying vec4 v_position;
attribute vec4 a_color;
varying vec4 v_color;
attribute vec3 a_normal;
varying vec3 v_normal;
attribute vec2 a_texcoord;
varying vec2 v_texcoord;

void main(void) {

    v_position = a_position;
    v_color = a_color;
    v_normal = a_normal;
    v_texcoord = a_texcoord;
    gl_Position = u_modelViewProjectionMatrix * v_position;
}

