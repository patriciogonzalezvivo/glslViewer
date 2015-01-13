#include "shapes.glsl"

uniform sampler2D tex0;
uniform vec2 tex0Resolution;

void main(void) {
	float rel = u_resolution.x/u_resolution.y;
	vec2 st = vec2(v_texcoord.x*rel,v_texcoord.y);

	float scale = 1.0;
	float texRel = tex0Resolution.x/tex0Resolution.y;
	vec4 color = texture2D(tex0,st*vec2(1.,texRel) );

	color += vec4(grid(st,10.0,0.02)*0.25);

	vec2 mouse = vec2(u_mouse.x*rel,u_mouse.y)/u_resolution;
	color += vec4(cross(mouse-st,0.01,0.001));
	color += vec4(box(mouse-st,abs(sin(u_time))*0.1,0.0015));

	gl_FragColor = color;
}
