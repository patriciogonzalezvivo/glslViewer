// Number: XVII
// Title: The Star
// Author: Patricio Gonzalez Vivo

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_resolution;

#include "../lib/fill.glsl"
#include "../lib/stroke.glsl"
#include "../lib/starSDF.glsl"
//GLOBAL_START
#include "../lib/raysSDF.glsl"
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
    color += stroke(raysSDF(st,8),.5,.15);
    float inner = starSDF(st.xy, 6, .09);
    float outer = starSDF(st.yx, 6, .09);
    color *= step(.7,outer);
    color += fill(outer, .5);
    color -= stroke(inner, .25, .06);
    color += stroke(outer, .6, .05);
    //END
    gl_FragColor = vec4(color,1.);
}