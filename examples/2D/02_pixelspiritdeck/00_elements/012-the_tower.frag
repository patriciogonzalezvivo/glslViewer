// Number: XVI
// Title: The Tower
// Author: Patricio Gonzalez Vivo

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_resolution;

#include "../lib/fill.glsl"
#include "../lib/stroke.glsl"
#include "../lib/rectSDF.glsl"
//GLOBAL_START
#include "../lib/flip.glsl"
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
    float rect = rectSDF(st, vec2(.5,1.));
    float diag = (st.x+st.y)*.5;
    color += flip(fill(rect,.6),
                  stroke(diag,.5,.01));
    //END
    gl_FragColor = vec4(color,1.);
}