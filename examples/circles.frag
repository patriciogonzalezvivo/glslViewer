#include "math.glsl"

const float antialias = 0.01;
const float speed = -0.12;
const float size = 2.0;

void main(){
    vec2 st = gl_FragCoord.xy/iResolution.xy;
    float aspect = iResolution.x/iResolution.y;
    st.x -= (iResolution.x-iResolution.y)/iResolution.x;
    st.x *= aspect;

	vec2 center = vec2(0.5);
	vec2 toExtreme = center-st;
	float radius = log(length(toExtreme))+iGlobalTime*speed;
	float angle = (PI+atan(toExtreme.y,toExtreme.x))/TWO_PI+iGlobalTime*speed;

	vec2 grid = vec2(fract(angle*10.*size),
					 fract(radius*1.5*size) );

	float pct = 1.0-distance(center,grid);
	vec3 color = vec3( smoothstep(0.5,0.6,pow(pct,3.0)) );

    gl_FragColor = vec4(color,
                        (st.x < 0. || st.x >= 1.0)? 0.0:1.0);
}
