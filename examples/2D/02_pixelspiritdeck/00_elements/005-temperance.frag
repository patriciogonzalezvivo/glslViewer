// Number: XIV
// Title: Temperance
// Author: Patricio Gonzalez Vivo

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_resolution;

#include "../lib/math.glsl"
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
    }
    //START
    float offset = cos(st.y*PI)*.15;
    color += stroke(st.x,.28+offset,.1);
    color += stroke(st.x,.5+offset,.1);
    color += stroke(st.x,.72+offset,.1);
    //END
    gl_FragColor = vec4(color,1.);
}