#ifdef GL_ES
precision mediump float;
#endif
uniform mat4 u_modelViewProjectionMatrix;

attribute vec4  a_position;
varying vec4    v_position;
attribute vec4  a_color;
varying vec4    v_color;
attribute vec3  a_normal;
varying vec3    v_normal;

uniform float   u_time;

#ifdef SCENE_HAS_SHADOWMAP
uniform mat4    u_lightMatrix;
varying vec4    v_lightcoord;
#endif

void main(void) {

    v_position = a_position;
    v_color = a_color;
    v_normal = a_normal;

#ifdef SCENE_HAS_SHADOWMAP
    v_lightcoord = u_lightMatrix * v_position;
#endif
    
    gl_PointSize = abs(sin(length(v_position.xyz) * 10. - u_time)) * 5.;
    gl_Position = u_modelViewProjectionMatrix * v_position;
}
