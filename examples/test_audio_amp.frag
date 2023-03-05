#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_tex0;
uniform vec2        u_tex0Resolution;

uniform vec2        u_resolution;

#include "../deps/lygia/draw/stroke.glsl"
#include "../deps/lygia/math/decimate.glsl"

void main (void) {
    vec3 color = vec3(0.0);
    vec2 pixel = 1.0/u_resolution.xy;
    vec2 st = gl_FragCoord.xy * pixel;
    float half_bar = 1./u_tex0Resolution.x;

    // hist
    vec2 grid = vec2(u_tex0Resolution.x * .5, 2.0);
    float his = texture2D(u_tex0, vec2(decimate(st.x, grid.x) + half_bar, 0.5) ).b;
    vec2 ipos = floor(st*grid);
    vec2 fpos = fract(st*grid);
    fpos.y = (mod(ipos.y,2.) == 0.)? 1.0-fpos.y : fpos.y;
    color += step(fpos.y, pow(his, 6.) * 10.) * step(0.5, fpos.x);
    
    gl_FragColor = vec4(color,1.0);
}
