#ifdef GL_ES
precision mediump float;
#endif

uniform float u_time;
uniform vec2 u_resolution;
uniform vec2 u_mouse;

varying vec2 v_texcoord;

#include "shapes.glsl"
#include "noise.glsl"

void main(void) {
	float rel = u_resolution.x/u_resolution.y;
	vec2 st = vec2(v_texcoord.x*rel,v_texcoord.y);

	vec3 color = mix(
			vec3(0.0,0.3,0.7),
			vec3(0.0,0.0,0.9),
			distance(st,vec2(0.5*rel,0.5))+noise(st*700.));

	color += vec3(grid(st,10.0,0.02)*0.25);

	vec2 mouse = vec2(u_mouse.x*rel,u_mouse.y)/u_resolution;
	color += vec3(cross(mouse-st,0.01,0.001));
	color += vec3(box(mouse-st,abs(sin(u_time))*0.1,0.0015));
	//color += vec3(circle(mouse-st,0.02,0.002));

	gl_FragColor.rgb = color;
	gl_FragColor.a = 1.0;//mouse.x;
}
