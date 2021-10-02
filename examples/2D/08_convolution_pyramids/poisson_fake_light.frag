
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_buffer0;
uniform sampler2D   u_convolutionPyramid0;

uniform vec2        u_resolution;
uniform vec2        u_mouse;
uniform float       u_time;


#include "../02_pixelspiritdeck/lib/ratio.glsl"
#include "../02_pixelspiritdeck/lib/stroke.glsl"
#include "../02_pixelspiritdeck/lib/rectSDF.glsl"
#include "../02_pixelspiritdeck/lib/circleSDF.glsl"

void main (void) {
    vec4 color = vec4(0.0, 0.0, 0.0, 0.0);
    vec2 pixel = 1.0/u_resolution;
    vec2 st = gl_FragCoord.xy * pixel;
    vec2 uv = ratio(st, u_resolution);

#if defined(CONVOLUTION_PYRAMID_0)
    color +=    stroke(uv.x, 0.5, 0.01) *
                vec4(0.999, 0.5, 0.1, 1.0);

    color -=    stroke(uv.x, 0.51, 0.015) *
                vec4(1.0, 1.0, 1.0, 0.9);
    
    color *=    stroke(uv.y, 0.5, 0.75);
    color *=    step(0.5, fract(uv.y + u_time));


    vec2 uv1 =  u_mouse * pixel - uv + 0.5;
    float c1 =  circleSDF(uv1);
    color +=    stroke(c1, 0.01, 0.001) *
                vec4(0.0, 1.0, 1.0, 1.0);

    float r =   rectSDF(uv, vec2(1.0));
    color.a +=  stroke(r, 1.0, 0.01);


#else
    color = texture2D(u_convolutionPyramid0, st);
#endif

    gl_FragColor = color;
}
