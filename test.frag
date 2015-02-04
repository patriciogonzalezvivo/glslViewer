#ifdef GL_ES
precision mediump float;
#endif

//	Default uniforms
uniform float u_time;
uniform vec2 u_mouse;
uniform vec2 u_resolution;

//	ShaderToy default uniforms
uniform float iGlobalTime;
uniform vec4 iMouse;
uniform vec3 iResolution;

//	Automatically add uniforms when you do:
//
//	$ piFrag test.frag -tex test.png
//
uniform sampler2D tex;
uniform vec2 texResolution;

//	Simple function that draws a rectangular shape
float rect (vec2 _position, vec2 _size) {
  _size = vec2(0.5)-_size*0.5;
  vec2 uv = smoothstep(_size,_size+vec2(0.0001),_position);
  uv *= smoothstep(_size,_size+vec2(0.0001),vec2(1.0)-_position);
  return uv.x*uv.y;
}

void main (void) {
	vec2 st = gl_FragCoord.xy/u_resolution.xy;

	//	Adjust aspect ratio
    float aspect = u_resolution.x/u_resolution.y;
    st.x -= (u_resolution.x-u_resolution.y)/u_resolution.x;
    st.x *= aspect;

    //	Set default color
	vec3 color = vec3(st.x, st.y, (1.0+sin(u_time))*0.5);

	//	Load image with proper aspect
	float imgAspect = texResolution.x/texResolution.y;
	vec4 img = texture2D(tex,st*vec2(1.,imgAspect)+vec2(0.0,-0.1));

	if ( texResolution != vec2(0.0) ) {
		// 	Blend the image to the default color 
		//	acording to the image alpha value
		color = mix(color,img.rgb,img.a);
	}
	
	//	Add a mouse pointer
	vec2 mouse = vec2(u_mouse.x*aspect,u_mouse.y)/u_resolution;
	color += vec3( rect(mouse-st, vec2(0.04,0.01)) + rect(mouse-st, vec2(0.01,0.04)) );

	//	Assign the resultant color
	gl_FragColor = vec4(color,1.0);

	//	Make the side of the screen transparent to see your terminal
	gl_FragColor.a = (st.x < 0. || st.x >= 1.0)? 0.0:1.0;
}
