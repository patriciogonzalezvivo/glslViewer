// Number: XV
// Title: The Devil
// Author: Patricio Gonzalez Vivo

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_resolution;

#include "../lib/stroke.glsl"
#include "../lib/circleSDF.glsl"
//GLOBAL_START
#include "../lib/starSDF.glsl"
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
    color += stroke(circleSDF(st),.8,.05);
    st.y = 1.-st.y;
    float s = starSDF(st.yx, 5, .1);
    color *= step(.7, s);
    color += stroke(s,.4,.1);
    //END
    gl_FragColor = vec4(color,1.);
}