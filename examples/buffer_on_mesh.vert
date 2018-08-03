#ifdef GL_ES
precision mediump float;
#endif

uniform mat4        u_modelViewProjectionMatrix;
uniform mat4        u_modelMatrix;
uniform mat4        u_viewMatrix;
uniform mat4        u_projectionMatrix;
uniform mat4        u_normalMatrix;

uniform sampler2D   u_buffer2;

uniform float       u_time;
uniform vec2        u_mouse;
uniform vec2        u_resolution;

attribute vec4      a_position;
attribute vec3      a_normal;
attribute vec2      a_texcoord;

varying vec4        v_position;
varying vec3        v_normal;
varying vec2        v_texcoord;

void main(void) {
    vec4 normal_depth = texture2D(u_buffer2, a_texcoord);
    v_position = a_position;
    v_position.xy -= 50.0;
    v_position.xy *= 0.03;
    v_position.x += 49.5;
    v_position.y += 49.7;
    v_position.z += normal_depth.a * 0.5;

    v_normal = a_normal;
    v_normal = normal_depth.xyz;

    v_texcoord = a_texcoord;

    gl_Position = u_modelViewProjectionMatrix * v_position;
}
