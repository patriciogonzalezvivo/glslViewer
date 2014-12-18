#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D tex0;
uniform vec2 tex0Resolution;

uniform float u_time;
uniform vec2 u_resolution;
uniform vec2 u_mouse;

varying vec2 v_texcoord;

#include "shapes.glsl"

void main(void) {
	float rel = u_resolution.x/u_resolution.y;
	vec2 st = vec2(v_texcoord.x*rel,v_texcoord.y);

	vec4 color = texture2D(tex0,v_texcoord);

	color += vec4(grid(st,10.0,0.02)*0.25);

	vec2 mouse = vec2(u_mouse.x*rel,u_mouse.y)/u_resolution;
	color += vec4(cross(mouse-st,0.01,0.001));
	color += vec4(box(mouse-st,abs(sin(u_time))*0.1,0.0015));

	gl_FragColor = color;
	// gl_FragColor.a *= mouse.x;
}
