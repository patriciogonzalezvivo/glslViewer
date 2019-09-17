// Number: X
// Title: Wheel of Fortune
// Author: Patricio Gonzalez Vivo

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_resolution;

#include "../lib/fill.glsl"
#include "../lib/stroke.glsl"
#include "../lib/circleSDF.glsl"
#include "../lib/polySDF.glsl"
#include "../lib/raysSDF.glsl"

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
    float sdf = polySDF(st.yx,8);
    color += fill(sdf,.5);
    color *= stroke(raysSDF(st,8),.5,.2);
    color *= step(.27,sdf);
    color += stroke(sdf,.2,.05);
    color += stroke(sdf,.6,.1);
    //END
    gl_FragColor = vec4(color,1.);
}