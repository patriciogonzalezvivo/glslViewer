#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_tex0;
uniform vec2        u_tex0Resolution;

uniform sampler2D   u_doubleBuffer0;

uniform vec2        u_resolution;

#include "lygia/color/palette/heatmap.glsl"

void main (void) {
    vec3 color = vec3(0.0);
    vec2 st = gl_FragCoord.xy/u_resolution.xy;
    vec2 pixel = 1.0/u_resolution;

    vec4 value = texture2D(u_tex0, vec2(st.x, 0.5) );

    // Frequency
    float fft = value.x;

    // Volume
    float vol = value.y;

#if defined(DOUBLE_BUFFER_0)
    color = texture2D(u_doubleBuffer0, st - vec2(0.0, 1.0) * pixel).rgb;
    
    color = mix(color, heatmap(fft), step(st.y, pixel.y));
#else
    color = texture2D(u_doubleBuffer0, st).rgb;

#endif

    gl_FragColor = vec4(color,1.0);
}
