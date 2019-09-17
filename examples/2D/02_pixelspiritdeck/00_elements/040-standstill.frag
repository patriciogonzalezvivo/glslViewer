// Title: Turning point
// Author: Patricio Gonzalez Vivo

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_resolution;

#include "../lib/stroke.glsl"
#include "../lib/rotate.glsl"
#include "../lib/polySDF.glsl"
#include "../lib/bridge.glsl"
#include "../lib/flip.glsl"

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
    st = (st-.5)*1.75+.5;
    //START
    st = rotate(st, radians(-60.));
    st.y = flip(st.y,step(.5,st.x));
    st.y += .25;
    float down = polySDF(st,3);
    st.y = 1.5-st.y;
    float top = polySDF(st,3);
    color += stroke(top,.4,.15);
    color = bridge(color,down,.4,.15);
    //END
    gl_FragColor = vec4(color,1.);
}