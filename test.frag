uniform float u_time;
uniform vec2 u_resolution;
uniform vec2 u_mouse;

varying vec2 v_texcoord;

void main(void) {
	vec3 color;

	color.r = u_mouse.x/u_resolution.x;
	color.g = u_mouse.y/u_resolution.y;

	gl_FragColor.rgb = color;
	gl_FragColor.a = 1.0;
}
