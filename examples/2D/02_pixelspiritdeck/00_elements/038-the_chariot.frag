// Number: VII
// Title: The Chariot
// Author: Patricio Gonzalez Vivo

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_resolution;

#include "../lib/stroke.glsl"
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
    float r1 = rectSDF(st, vec2(1.));
    float r2 = rectSDF(rotate(st,radians(45.)), 
                       vec2(1.));
    float inv = step(.5,(st.x+st.y)*.5);
    inv = flip(inv,step(.5,.5+(st.x-st.y)*.5));
    float w = .075;
    color += stroke(r1,.5,w) + stroke(r2,.5,w);
    float bridges = mix(r1,r2,inv);
    color = bridge(color, bridges, .5, w);
    //END
    gl_FragColor = vec4(color,1.);
}