
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_target;
uniform vec2        u_targetResolution;

uniform sampler2D   u_source;
uniform sampler2D   u_source_mask;
uniform vec2        u_sourceResolution;

uniform sampler2D   u_buffer0;
uniform sampler2D   u_buffer1;
uniform sampler2D   u_buffer2;
uniform sampler2D   u_buffer3;

uniform sampler2D   u_convolutionPyramid0;
uniform sampler2D   u_convolutionPyramidTex0;
uniform sampler2D   u_convolutionPyramidTex1;
uniform bool        u_convolutionPyramidUpscaling;

uniform vec2        u_resolution;
uniform vec2        u_mouse;

const vec3  h1      = vec3(1.0334, 0.6836, 0.1507);
const float h2      = 0.0270;
const vec2  g       = vec2(0.7753, 0.0312);

#define saturate(x) clamp(x, 0.0, 1.0)
#define absi(x)     ( (x < 0)? x * -1 : x )

vec3 laplace(sampler2D tex, vec2 st, vec2 pixel) {
    return texture2D(tex, st).rgb * 4.0
            - texture2D(tex, st + vec2(-1.0,  0.0) * pixel).rgb
            - texture2D(tex, st + vec2( 0.0, -1.0) * pixel).rgb
            - texture2D(tex, st + vec2( 0.0,  1.0) * pixel).rgb
            - texture2D(tex, st + vec2( 1.0,  0.0) * pixel).rgb;
}

void main (void) {
    vec4 color = vec4(0.0,0.0,0.0,1.0);
    vec2 st = gl_FragCoord.xy/u_resolution;
    vec2 pixel = 1.0/u_resolution;
    float pct = step(st.y, u_mouse.y/u_resolution.y);

#if defined(BUFFER_0)
// // Scale and position SOURCE and MASK (as the alpha) )
    vec2 translate = vec2(0.0, 0.1);
    float scale = 2.;

    vec2 uv = (st-0.5) * vec2(u_sourceResolution.y/u_sourceResolution.x,1.0) * scale + 0.5;
    uv += translate;

    color.rgb = texture2D(u_source, saturate(uv)).rgb;
    color.a *= texture2D(u_source_mask, saturate(uv)).r;

#elif defined(BUFFER_1)
    // TARGET GRADIENT extraction with LAPLACE
    color.rgb = laplace(u_buffer0, st, pixel * 2.0);
    color.rgb = color.rgb * 0.5 + 0.5;

#elif defined(CONVOLUTION_PYRAMID_0)
    // COPY TARGET with out MASK area
    color = texture2D(u_target, st);
    color *= 1.0-texture2D(u_buffer0, st).a;

#elif defined(CONVOLUTION_PYRAMID_ALGORITHM)
    color = vec4(0.0);

    if (!u_convolutionPyramidUpscaling) {
        for (int dy = -2; dy <= 2; dy++) {
            for (int dx = -2; dx <= 2; dx++) {
                vec2 uv = st + vec2(float(dx), float(dy)) * pixel;
                if (uv.x <= 0.0 || uv.x >= 1.0 || uv.y <= 0.0 || uv.y >= 1.0)
                    continue;
                color += texture2D(u_convolutionPyramidTex0, saturate(uv)) * h1[ absi(dx) ] * h1[ absi(dy) ];
            }
        }
    }
    else {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                vec2 uv = st + vec2(float(dx), float(dy)) * pixel;
                color += texture2D(u_convolutionPyramidTex0, saturate(uv)) * g[ absi(dx) ] * g[ absi(dy) ];
            }
        }

        float msk = texture2D(u_buffer0, st).a;
        for (int dy = -2; dy <= 2; dy++) {
            for (int dx = -2; dx <= 2; dx++) {
                vec2 offset = vec2(float(dx), float(dy)) * pixel;
                color += texture2D(u_convolutionPyramidTex1, saturate(st + offset * 5.0)) * h2 * h1[ absi(dx) ] * h1[ absi(dy) ] ;
                vec3 src = texture2D(u_buffer1, saturate(st + offset)).rgb - 0.5;
                color.rgb += src * msk * .08 * pct;
            }
        }
    }

    color = (color.a == 0.0)? color : vec4(color.rgb/color.a, 1.0);

#else
    color = texture2D(u_convolutionPyramid0, st);
    
    // vec4 naive = mix( texture2D(u_target,st), texture2D(u_buffer0,st), texture2D(u_buffer0,st).a );
    // color = mix(color, naive, step(st.x, u_mouse.x/u_resolution.x));

#endif

    gl_FragColor = color;
}
