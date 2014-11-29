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
	
	vec3 color = vec3(0.0,0.0,
			noise(vec3(st.y,st.x,cos(u_time))*20.));

	color += grid(0.03,20.0,st).rgb*0.25;
	color += grid(0.012,10.0,st).rgb*0.5;

	vec2 mouse = vec2(u_mouse.x*rel,u_mouse.y)/u_resolution;
	color += cross(0.0006,0.01,mouse-st).rgb;

	gl_FragColor.rgb = color;
	gl_FragColor.a = 0.2;
}
