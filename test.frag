#ifdef GL_ES
precision mediump float;
#endif

uniform float u_time;
uniform vec2 u_resolution;
uniform vec2 u_mouse;

varying vec2 v_texcoord;

void main(void) {
	float rel = u_resolution.x/u_resolution.y;
	vec2 st = vec2(v_texcoord.x*rel,v_texcoord.y);
	
	vec3 color = vec3(abs(cos(u_time*10.0))*st.x,
			abs(sin(u_time*7.0)),
			abs(-cos(u_time*11.0))*st.y);

	// Mouse
	//
	float rad = 0.01;
	vec2 mouse = vec2(u_mouse.x*rel,u_mouse.y)/u_resolution;
	vec2 diff = mouse-st;
	float dist = length(diff);
	if(dist < rad){
		float lineWidth = 0.0006;
		if(abs(diff.x) < lineWidth || abs(diff.y) < lineWidth+0.00005 ){
			color += vec3(1.0);
		}
		//float pct = pow(dist/rad,2.0);
		//color.rgb += vec3(1.0-pct);
	}

	// Grid
	//	
	float lineWith = 0.012;
	vec2 grid = fract(st*10.);
	if (grid.x < lineWith || grid.y < lineWith){
		color.x += smoothstep(0.0,lineWith,grid.x);
		color.y += smoothstep(0.0,lineWith,grid.y);
	}

	gl_FragColor.rgb = color;
	gl_FragColor.a = 0.2;
}
