// Title: The Shadow
// Author: Patricio Gonzalez Vivo

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_resolution;

#include "../lib/rectSDF.glsl"
#include "../lib/rotate.glsl"
#include "../lib/fill.glsl"

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
    };
    //START
    st = rotate(vec2(st.x,1.0-st.y),
                radians(45.));
    vec2 s = vec2(1.);
    color += fill(rectSDF(st-.025,s),.4);
    color += fill(rectSDF(st+.025,s),.4);
    color *= step(.38,rectSDF(st+.025,s));
    //END
    gl_FragColor = vec4(color,1.);
}