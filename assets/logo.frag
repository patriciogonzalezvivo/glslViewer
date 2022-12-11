#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_resolution;

#include "lygia/math/const.glsl"
#include "lygia/math/saturate.glsl"
#include "lygia/space/ratio.glsl"
#include "lygia/space/scale.glsl"
#include "lygia/space/rotate.glsl"
#include "lygia/draw/fill.glsl"
#include "lygia/draw/stroke.glsl"
#include "lygia/sdf.glsl"

void main() {
    vec3 color = vec3(0.);
    vec2 pixel = 1.0/u_resolution;
    vec2 st = gl_FragCoord.xy * pixel;

    st = ratio(st, u_resolution);

    color += stroke( circleSDF(st), 0.8, 0.1);

    // color += stroke( rectSDF( rotate( st, PI * 0.25), vec2(1.)), 0.65, 0.1);

    st = rotate( st, PI * 0.25);
    color += stroke( rectSDF( st + vec2(-0.09), vec2(1.)), 0.18, 0.1);
    color += fill( rectSDF( st + vec2(0.0, 0.16), vec2(1., 0.2)), 0.4);
    color += fill( rectSDF( st.yx + vec2(0.0, 0.16), vec2(1., 0.2)), 0.4);

    gl_FragColor = vec4(color, color.r);
}