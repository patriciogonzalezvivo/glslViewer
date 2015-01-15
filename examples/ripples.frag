#include "math.glsl"

const float rignsNumber = 4.0;
const float speed = 1.0;
const float black = 0.9;
const float antialias = 0.1;

void main(){
    vec2 st = gl_FragCoord.xy/iResolution.xy;
    float aspect = iResolution.x/iResolution.y;
    st.x -= (iResolution.x-iResolution.y)/iResolution.x;
    st.x *= aspect;

	float pct = 1.-distance(vec2(0.5),st)/0.5;
    float waves = abs(cos(fract( pct*rignsNumber) * PI + iGlobalTime*speed ));
	float threshold = smoothstep(black-antialias, black+antialias, waves);
	vec3 color = vec3(threshold);

    gl_FragColor = vec4(color,
                        (st.x < 0. || st.x >= 1.0)? 0.0:1.0);
}
