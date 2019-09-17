// Title: The Diamond
// Author: Patricio Gonzalez Vivo

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_resolution;

#include "../lib/fill.glsl"
#include "../lib/stroke.glsl"
#include "../lib/triSDF.glsl"
//GLOBAL_START
float rhombSDF(vec2 st) {
    return max(triSDF(st),
               triSDF(vec2(st.x,1.-st.y)));
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
    float sdf = rhombSDF(st);
    color += fill(sdf,.425);
    color += stroke(sdf,.5,.05);
    color += stroke(sdf,.6,.03);
    //END
    gl_FragColor = vec4(color,1.);
}