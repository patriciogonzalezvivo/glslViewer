// Title: Ripples
// Author: Patricio Gonzalez Vivo

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_resolution;

#include "../lib/rectSDF.glsl"
#include "../lib/rotate.glsl"
#include "../lib/stroke.glsl"

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
    };
    st = (st-.5)*.8+.5;
    //START
    st = rotate(st,radians(-45.))-.08;
    for (int i = 0; i < 4; i++) {
        float r = rectSDF(st, vec2(1.));
        color += stroke(r, .19, .04);
        st += .05;
    }
    //END
    gl_FragColor = vec4(color,1.);
}