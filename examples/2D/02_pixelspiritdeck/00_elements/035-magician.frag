// Number: I
// Title: The Magician
// Author: Patricio Gonzalez Vivo

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_resolution;

#include "../lib/stroke.glsl"
#include "../lib/circleSDF.glsl"
#include "../lib/flip.glsl"

//GLOBAL_START
vec3 bridge(vec3 c,float d,float s,float w) {
    c *= 1.-stroke(d,s,w*2.);
    return c + stroke(d,s,w);
}
//GLOBAL_END

void main() {
    vec3 color = vec3(0.);
    vec2 st = gl_FragCoord.xy/u_resolution;
    st = (st-.5)*1.1912+.5;
    if (u_resolution.y > u_resolution.x ) {
        st.y *= u_resolution.y/u_resolution.x;
        st.y -= (u_resolution.y*.5-u_resolution.x*.5)/u_resolution.x;
    } else {
        st.x *= u_resolution.x/u_resolution.y;
        st.x -= (u_resolution.x*.5-u_resolution.y*.5)/u_resolution.y;
    }
    //START
    st.x = flip(st.x,step(.5,st.y));
    vec2 offset = vec2(.15,.0);
    float left = circleSDF(st+offset);
    float right = circleSDF(st-offset);
    color += stroke(left, .4, .075);
    color = bridge(color, right, .4,.075);
    //END
    gl_FragColor = vec4(color,1.);
}