#ifdef GL_ES
precision mediump float;
#endif

uniform float u_time;
uniform vec2 u_resolution;
uniform vec2 u_mouse;

varying vec2 v_texcoord;

void main(void) {
	vec3 color = vec3(abs(sin(u_time)),
			abs(sin(u_time*0.5)),
			0.1);

	float dist = distance(abs(u_mouse/u_resolution),v_texcoord);
	if(dist < 10.0){
		color.r = dist/10.0;
	}

	// Grid
	//
	float rel = u_resolution.x/u_resolution.y;	
	vec2 grid = fract(vec2(v_texcoord.x*rel,v_texcoord.y)*10.);
	float lineWith = 0.0155555;
	if (grid.x < lineWith || grid.y < lineWith){
		color.x += cos(smoothstep(0.,lineWith,grid.x));
		color.y += cos(smoothstep(0.,lineWith,grid.y));
	}

	gl_FragColor.rgb = color;
	gl_FragColor.a = 1.0;
}
