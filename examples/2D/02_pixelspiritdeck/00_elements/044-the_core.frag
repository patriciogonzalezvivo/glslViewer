// Title: The Core
// Author: Patricio Gonzalez Vivo

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_resolution;

#include "../lib/math.glsl"
#include "../lib/fill.glsl"
#include "../lib/rotate.glsl"
#include "../lib/rhombSDF.glsl"
#include "../lib/starSDF.glsl"

//GLOBAL_START
#include "../lib/scale.glsl"
//GLOBAL_END

void main() {
    vec3 color = vec3(0.);
    vec2 st = gl_FragCoord.xy/u_resolution;
    st = (st-.5)*1.1912+.5;
    st = mix(vec2((st.x*u_resolution.x/u_resolution.y)-(u_resolution.x*.5-u_resolution.y*.5)/u_resolution.y,st.y), 
             vec2(st.x,st.y*(u_resolution.y/u_resolution.x)-(u_resolution.y*.5-u_resolution.x*.5)/u_resolution.x), 
             step(u_resolution.x,u_resolution.y));
    st = (st-.5)*1.2+.5;
    //START
    float star = starSDF(st,8,.063);
    color += fill(star,1.22);
    float n = 8.;
    float a = TAU/n;
    for (float i = 0.; i < n; i++) {
        vec2 xy = rotate(st,0.39+a*i);
        xy = scale(xy,vec2(1.,.72));
        xy.y -= .125;
        color *= step(.235,rhombSDF(xy));
    }
    //END
    gl_FragColor = vec4(color,1.);
}
