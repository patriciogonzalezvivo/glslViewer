#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_tex0;
uniform vec2        u_tex0Resolution;

uniform vec2        u_resolution;

#include "lygia/color/palette/heatmap.glsl"
#include "lygia/math/decimate.glsl"

void main (void) {
    vec3 color = vec3(0.0);
    vec2 pixel = 1.0/u_resolution.xy;
    vec2 st = gl_FragCoord.xy * pixel;
    float half_bar = 1./u_tex0Resolution.x;

    // fft
    float bar = decimate(st.x, 25.);
    float fft = texture2D(u_tex0, vec2(bar + half_bar, 0.5) ).r;
    color += heatmap(bar) * step(st.y, fft);
    
    gl_FragColor = vec4(color,1.0);
}
