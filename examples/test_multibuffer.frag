#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_buffer0;
uniform sampler2D   u_buffer1;

uniform sampler2D   u_tex0;
uniform vec2 u_tex0Resolution;

uniform vec2        u_resolution;
uniform vec2        u_mouse;
uniform float       u_time;

varying vec2        v_texcoord;

void main() {
    vec3 color = vec3(0.0);
    vec2 st = v_texcoord;

#ifdef BUFFER_0
    color.g = abs(sin(u_time));

    // color = mix(color, 
    //             texture2D(u_buffer1, st).rgb,
    //             step(0.75, st.y));

#elif defined( BUFFER_1 )
    color.r = abs(sin(u_time));

    // color = mix(color, 
    //             texture2D(u_buffer0, st).rgb,
    //             1.-step(0.75, st.y));

#else
    // color.rg = st;
    color.b = abs(sin(u_time));

    color = mix(color, 
                mix(texture2D(u_buffer0, st).rgb, 
                    texture2D(u_buffer1, st).rgb, 
                    step(0.5, st.x) ), 
                step(0.5, st.y));
#endif

    if ( u_tex0Resolution != vec2(0.0) ){
    	float imgAspect = u_tex0Resolution.x/u_tex0Resolution.y;
		vec4 img = texture2D(u_tex0,st*vec2(1.,imgAspect));
	    color = mix(color,img.rgb,img.a);
    }
    
    gl_FragColor = vec4(color, 1.0);
}