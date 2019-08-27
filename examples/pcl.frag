#ifdef GL_ES
precision mediump float;
#endif

uniform vec3    u_light;

varying vec4 v_position;
varying vec4 v_color;
varying vec3 v_normal;

void main(void) {
    vec3 color = vec3(1.0);
    color = v_color.rgb;
    gl_FragColor = vec4(color, 1.0);
}
