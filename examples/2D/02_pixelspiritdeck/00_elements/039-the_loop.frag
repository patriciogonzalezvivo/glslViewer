// Title: The Loop
// Author: Patricio Gonzalez Vivo

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_resolution;

#include "../lib/rectSDF.glsl"
#include "../lib/rotate.glsl"
#include "../lib/flip.glsl"
#include "../lib/bridge.glsl"

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
    float inv = step(.5,st.y);
    st = rotate(st,radians(-45.))-.2;
    st = mix(st,.6-st,step(.5,inv));
    for (int i = 0; i < 5; i++) {
        float r = rectSDF(st, vec2(1.));
        float s = .25;
        s -= abs(float(i)*.1-.2);
        color = bridge(color, r,s,.05);
        st += .1;
    }
    //END
    gl_FragColor = vec4(color,1.);
}