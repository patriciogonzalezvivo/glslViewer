// Title: The Cauldron
// Author: Patricio Gonzalez Vivo

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_resolution;

#include "../lib/math.glsl"
#include "../lib/stroke.glsl"
#include "../lib/rotate.glsl"
#include "../lib/vesicaSDF.glsl"

void main() {
    vec3 color = vec3(0.);
    vec2 st = gl_FragCoord.xy/u_resolution;
    st = (st-.5)*1.1912+.5;
    st = mix(vec2((st.x*u_resolution.x/u_resolution.y)-(u_resolution.x*.5-u_resolution.y*.5)/u_resolution.y,st.y), 
             vec2(st.x,st.y*(u_resolution.y/u_resolution.x)-(u_resolution.y*.5-u_resolution.x*.5)/u_resolution.x), 
             step(u_resolution.x,u_resolution.y));
    //START
    float n = 12.;
    float a = TAU/n;
    for (float i = 0.; i < n; i++) {
        vec2 xy = rotate(st,a*i);
        xy.y -= .189;
        float vsc = vesicaSDF(xy,.3);
        color *= 1.-stroke(vsc,.45,0.1)*
                 step(.5,xy.y);
        color += stroke(vsc,.45,0.05);
    }
    //END
    gl_FragColor = vec4(color,1.);
}
