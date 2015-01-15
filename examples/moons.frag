#include "math.glsl"

const float antialias = 0.01;
const float speedR = 0.2;
const float speedA = 0.0;
const float size = 1.1;

float circle(vec2 _st, float _size, float _antialias){
	float pct = 1.0-distance(vec2(0.5),_st);
	return smoothstep(_size-_antialias,_size+_antialias,pct);
}

void main(){
    vec2 st = gl_FragCoord.xy/iResolution.xy;
    float aspect = iResolution.x/iResolution.y;
    st.x -= (iResolution.x-iResolution.y)/iResolution.x;
    st.x *= aspect;

	vec2 center = vec2(0.5);
	vec2 toExtreme = center-st;
	float radius = log(length(toExtreme))+iGlobalTime*speedR;
	float angle = (PI+atan(toExtreme.y,toExtreme.x))/TWO_PI+iGlobalTime*speedA;

	radius += angle*(PHI-1.0);
	vec2 grid = vec2(	fract(angle*10.*size),
					 	fract(radius*1.5*size) );

	vec2 shadowGrid = vec2(	fract( (angle+radius*0.04+cos(angle*PI)*0.0127)*10.*size),
							fract( radius*1.5*size) );

	float moon = clamp(circle(grid,0.8,antialias) - circle(shadowGrid,0.77,antialias), 0.0,1.0 );
	vec3 color = vec3( moon );

    gl_FragColor = vec4(color,
                        (st.x < 0. || st.x >= 1.0)? 0.0:1.0);
}
