// Title: Opposite
// Author: Patricio Gonzalez Vivo

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_resolution;

#include "../lib/rectSDF.glsl"
#include "../lib/rotate.glsl"
#include "../lib/fill.glsl"
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
    };
    //START
    st = rotate(st,radians(-45.));
    vec2 s = vec2(1.);
    float o = .05;
    color += flip(fill(rectSDF(st-o,s),.4),
                  fill(rectSDF(st+o,s),.4));
    //END
    gl_FragColor = vec4(color,1.);
}