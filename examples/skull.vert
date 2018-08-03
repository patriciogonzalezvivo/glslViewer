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
attribute vec4 a_color;
attribute vec3 a_normal;
attribute vec2 a_texcoord;

varying vec4 v_position;
varying vec4 v_color;
varying vec3 v_normal;
varying vec2 v_texcoord;

mat4 rotate4dZ(in float psi){
    return mat4(
        vec4(cos(psi),-sin(psi),0.,0),
        vec4(sin(psi),cos(psi),0.,0.),
        vec4(0.,0.,1.,0.),
        vec4(0.,0.,0.,1.));
}

void main(void) {
    v_position = rotate4dZ(u_time) * a_position;
    
    v_color = a_color;
    v_normal = a_normal;
    v_texcoord = a_texcoord;

    gl_Position = u_modelViewProjectionMatrix * v_position;
}
