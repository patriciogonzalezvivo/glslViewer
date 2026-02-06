
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_tex0;
uniform vec2        u_tex0Resolution;
uniform vec2        u_resolution;

uniform sampler2D   u_pyramid0;
uniform sampler2D   u_pyramidTex0;
uniform sampler2D   u_pyramidTex1;
uniform bool        u_pyramidUpscaling;

const vec3  h1      = vec3(1.0334, 0.6836, 0.1507);
const float h2      = 0.0270;
const vec2  g       = vec2(0.7753, 0.0312);

#include "lygia/math/saturate.glsl"

void main (void) {
    vec4 color = vec4(0.0);
    vec2 st = gl_FragCoord.xy/u_resolution;

#if defined(PYRAMID_0)
    color = texture2D(u_tex0, st);

#else
    color = texture2D(u_pyramid0, st);

#endif

    gl_FragColor = color;
}
