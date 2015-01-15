#include "math.glsl"
#include "color.glsl"

const float speed = 0.5;

void main(){
    vec2 st = gl_FragCoord.xy/iResolution.xy;
    float aspect = iResolution.x/iResolution.y;
    st.x -= (iResolution.x-iResolution.y)/iResolution.x;
    st.x *= aspect;

	vec2 center = vec2(0.5);
	vec2 toExtreme = center-st;
	float radius = length(toExtreme)/0.5;
	float angle = (PI+atan(toExtreme.y,toExtreme.x))/TWO_PI;  
	vec3 color = hsv2rgb_smooth(vec3(angle+iGlobalTime*speed, 
									 radius, 
									 smoothstep(0.0,0.1,1.0-radius)));

    gl_FragColor = vec4(color,
                        (st.x < 0. || st.x >= 1.0)? 0.0:1.0);
}
