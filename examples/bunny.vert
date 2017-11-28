#ifdef GL_ES
precision mediump float;
#endif

uniform mat4 u_modelMatrix;
uniform mat4 u_viewMatrix;
uniform mat4 u_projectionMatrix;
uniform mat4 u_modelViewProjectionMatrix;
uniform mat3 u_normalMatrix;

uniform float u_time;
uniform vec2 u_mouse;
uniform vec2 u_resolution;

attribute vec4 a_position;
attribute vec4 a_color;
attribute vec3 a_normal;
attribute vec2 a_texcoord;

varying vec4 v_position;
varying vec3 v_normal;

void main(void) {
    v_position = u_modelViewProjectionMatrix * a_position;
    gl_Position = v_position;

    v_normal = a_normal;
}
