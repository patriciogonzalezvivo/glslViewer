// Title: The Link
// Author: Patricio Gonzalez Vivo

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_resolution;

#include "../lib/fill.glsl"
#include "../lib/stroke.glsl"
#include "../lib/bridge.glsl"
#include "../lib/rotate.glsl"
#include "../lib/rectSDF.glsl"
#include "../lib/rhombSDF.glsl"

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
    st = (st-.5)*1.2+.5;
    //START;
    st = st.yx;
    st.x = mix(1.-st.x,st.x,step(.5,st.y));
    vec2 o = vec2(.1,.0);
    vec2 s = vec2(1.);
    float a = radians(45.);
    float l = rectSDF(rotate(st+o,a),s);
    float r = rectSDF(rotate(st-o,-a),s);
    color += stroke(l,.3,.1);
    color = bridge(color, r,.3,.1);
    color += fill(rhombSDF(abs(st.yx-
                           vec2(.0,.5))),
                 .1);
    //END
    gl_FragColor = vec4(color,1.);
}