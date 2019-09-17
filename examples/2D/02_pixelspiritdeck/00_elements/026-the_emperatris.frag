// Number: III
// Title: The Empress
// Author: Patricio Gonzalez Vivo

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_resolution;

#include "../lib/fill.glsl"
//GLOBAL_START
#include "../lib/polySDF.glsl"
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
    st = (st-.5)*1.1+.5;
    //START
    float d1 = polySDF(st,5);
    vec2 ts = vec2(st.x,1.-st.y);
    float d2 = polySDF(ts,5);
    color += fill(d1,.75) *
             fill(fract(d1*5.),.5);
    color -= fill(d1,.6) *
             fill(fract(d2*4.9),.45);
    //END
    gl_FragColor = vec4(color,1.);
}