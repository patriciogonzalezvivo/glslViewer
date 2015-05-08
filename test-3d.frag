#ifdef GL_ES
precision mediump float;
#endif

//	Default uniforms
uniform float u_time;
uniform vec2 u_mouse;
uniform vec2 u_resolution;

varying vec4 v_position;
// varying vec4 v_color;
varying vec3 v_normal;
// varying vec2 v_texcoord;

//	Simple function that draws a rectangular shape
float rect (vec2 pos, vec2 size) {
  size = vec2(0.5)-size*0.5;
  vec2 uv = smoothstep(size,size+vec2(0.0001),pos);
  uv *= smoothstep(size,size+vec2(0.0001),vec2(1.0)-pos);
  return uv.x*uv.y;
}

void main (void) {
	vec2 st = gl_FragCoord.xy/u_resolution.xy;
	float aspect = u_resolution.x/u_resolution.y;
    st.x *= aspect;

    vec3 color = (v_normal-0.5)*2.;

	//	Add a mouse cursor
	vec2 mousePos = st-vec2(u_mouse.x*aspect,u_mouse.y)/u_resolution+vec2(0.5);
	color += vec3( rect(mousePos, vec2(0.03,0.005)) + rect(mousePos, vec2(0.005,0.03)) );

	//	Assign the resultant color
	gl_FragColor = vec4(color,1.0);
}
