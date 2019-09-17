// Title: The Oak
// Author: Patricio Gonzalez Vivo

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_resolution;

#include "../lib/fill.glsl"
#include "../lib/stroke.glsl"
#include "../lib/rotate.glsl"
#include "../lib/rectSDF.glsl"

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
    st = (st-.5)*1.3+.5;
    //START
    st = rotate(st,radians(45.));
    float r1 = rectSDF(st,vec2(1.));
    float r2 = rectSDF(st+.15,vec2(1.));
    color += stroke(r1,.5,.05);
    color *= step(.325,r2);
    color += stroke(r2,.325,.05) *
             fill(r1,.525);
    color += stroke(r2,.2,.05);
    //END
    gl_FragColor = vec4(color,1.);
}