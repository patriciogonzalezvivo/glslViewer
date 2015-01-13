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
	vec4 color = vec4(0.0);

    vec2 vgrid = fract(st*10.);
    float lineWidth = 0.0064;	
    
    
    if( (vgrid.x > 0.5-lineWidth && vgrid.x < 0.5+lineWidth) ||
        (vgrid.y > 0.5-lineWidth && vgrid.y < 0.5+lineWidth) ){
	    color += vec4(0.3);
    }

    if(cross(vgrid-vec2(0.5),
            noise(st*fract(u_time*0.05)*10.0),
            0.01) == 1.0){
        color = vec4(0.0);
    }

    if(cross(vgrid-vec2(0.5),0.045,0.0072) == 1.0){
        color += vec4(1.0);
    }

	vec2 mouse = vec2(u_mouse.x*rel,u_mouse.y)/u_resolution;
	color += vec4(cross(mouse-st,0.01,0.00072))*0.9;
	color += vec4(box(mouse-st,abs(sin(u_time))*0.1,0.0014));
	//color += vec3(circle(mouse-st,0.02,0.002));

	gl_FragColor = color;
}
