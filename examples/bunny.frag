#ifdef GL_ES
precision mediump float;
#endif

uniform float u_time;
uniform vec2 u_mouse;
uniform vec2 u_resolution;

varying vec4 v_position;
varying vec3 v_normal;
varying vec2 v_texcoord;

void main (void) {
    vec3 color = vec3(0.0);

    color = (v_normal+1.0) * 0.5;

	gl_FragColor = vec4(color,1.0);
}
