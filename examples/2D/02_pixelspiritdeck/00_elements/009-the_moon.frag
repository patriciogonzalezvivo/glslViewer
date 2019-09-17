// Number: XVIII
// Title: The Moon
// Author: Patricio Gonzalez Vivo

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_resolution;

#include "../lib/fill.glsl"
#include "../lib/circleSDF.glsl"
//GLOBAL_START
#ifndef FNC_FILL
#define FNC_FILL
float fill(float x, float size) {
    return 1.-step(size, x);
}
#endif
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
    color += fill(circleSDF(st),.65);
    vec2 offset = vec2(.1,.05);
    color -= fill(circleSDF(st-offset),.5);
    //END
    gl_FragColor = vec4(color,1.);
}