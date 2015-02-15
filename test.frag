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
    vec3 color = vec3(0.0);

	//	Adjust aspect ratio
    float aspect = u_resolution.x/u_resolution.y;
    st.x *= aspect;

    //  Map the red and green channels to the X and Y values
	color = vec3(st.x, st.y, (1.0+sin(u_time))*0.5);

	//	Load image and fix their aspect ratio
	float imgAspect = texResolution.x/texResolution.y;
	vec4 img = texture2D(tex,st*vec2(1.,imgAspect)+vec2(0.0,-0.1));

	color = mix(color,img.rgb,img.a);
	
	//	Add a mouse cursor
	vec2 mousePos = st-vec2(u_mouse.x*aspect,u_mouse.y)/u_resolution+vec2(0.5);
	color += vec3( rect(mousePos, vec2(0.03,0.005)) + rect(mousePos, vec2(0.005,0.03)) );

	//	Assign the resultant color
	gl_FragColor = vec4(color,1.0);
}
