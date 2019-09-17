// Number: V
// Title: The Hierophant
// Author: Patricio Gonzalez Vivo

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_resolution;

#include "../lib/fill.glsl"
#include "../lib/stroke.glsl"
#include "../lib/rectSDF.glsl"
//GLOBAL_START
float crossSDF(vec2 st, float s) {
    vec2 size = vec2(.25, s);
    return min( rectSDF(st,size.xy),
                rectSDF(st,size.yx));
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
    st = (st-.5)*1.2+.5;
    //START
    float rect = rectSDF(st,vec2(1.));
    color += fill(rect,.5);
    float cross = crossSDF(st, 1.);
    color *= step(.5,fract(cross*4.));
    color *= step(1.,cross);
    color += fill(cross,.5);
    color += stroke(rect,.65,.05);
    color += stroke(rect,.75,.025);
    //END
    gl_FragColor = vec4(color,1.);
}