
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_target;
uniform sampler2D   u_source;
uniform sampler2D   u_source_mask;
uniform vec2        u_sourceResolution;

uniform sampler2D   u_convolutionPyramid0;

uniform vec2        u_resolution;
uniform vec2        u_mouse;
uniform float       u_time;

#define saturate(x) clamp(x, 0.0, 1.0)

void main (void) {
    vec4 color = vec4(0.0,0.0,0.0,1.0);
    vec2 st = gl_FragCoord.xy/u_resolution;

    // Target
    vec4 trg = texture2D(u_target, st);

    // Source (needs scale and translation)
    float src_scale = 2.5;
    vec2 src_pos = vec2(0.0, 0.1);
    src_pos = 1.0-u_mouse/u_resolution * 2.0;
    vec2 src_uv = ((st-0.5) * vec2(u_sourceResolution.y/u_sourceResolution.x,1.0) * src_scale + 0.5) + src_pos;
    vec4 src = texture2D(u_source, saturate(src_uv));
    float msk = texture2D(u_source_mask, saturate(src_uv)).r;

#if defined(CONVOLUTION_PYRAMID_0)
    color.xyz = (trg.rgb - src.rgb) * 0.5 + 0.5;
    color *= 1.0-msk;

#else
    vec3 er = texture2D(u_convolutionPyramid0, st).xyz * 2.0 - 1.0;
    color.rgb = er.rgb + src.rgb;

#endif

    gl_FragColor = color;
}