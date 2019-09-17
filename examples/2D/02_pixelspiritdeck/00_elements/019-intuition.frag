// Title: Intuition
// Author: Patricio Gonzalez Vivo

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_resolution;

#include "../lib/fill.glsl"
#include "../lib/triSDF.glsl"
//GLOBAL_START
#include "../lib/rotate.glsl"
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
    st.x -= 0.08;
    st.y -= 0.1;
    //START
    st = rotate(st, radians(-25.));
    float sdf = triSDF(st);
    sdf /= triSDF(st+vec2(0.,.2));
    color += fill(abs(sdf),.56);
    //END
    gl_FragColor = vec4(color,1.);
}