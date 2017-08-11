#ifdef GL_ES
precision mediump float;
#endif

uniform float u_time;
uniform vec2 u_mouse;
uniform vec2 u_resolution;

varying vec4 v_position;
varying vec3 v_normal;

void main (void) {
    vec3 color = (v_normal+1.0)*0.5;

	gl_FragColor = vec4(color,1.0);
}
