
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_tex0;
uniform vec2        u_tex0Resolution;

uniform vec2        u_resolution;
uniform vec2        u_mouse;

uniform sampler2D   u_buffer0;
uniform sampler2D   u_buffer1;

uniform sampler2D   u_convolutionPyramid0;
uniform sampler2D   u_convolutionPyramidTex0;
uniform sampler2D   u_convolutionPyramidTex1;
uniform bool        u_convolutionPyramidUpscaling;

// Laplacian Integrator
const vec3  h1      = vec3(.7, 0.5, 0.15);
const float h2      = .2;
const vec2  g       = vec2(0.547, 0.175);

#define saturate(x) clamp(x, 0.0, 1.0)
#define absi(x)     ( (x < 0)? x * -1 : x )
vec3 laplacian(sampler2D tex, vec2 st, vec2 pixel, float pixelpad);

void main (void) {
    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    vec2 st = gl_FragCoord.xy/u_resolution;
    vec2 pixel = 1.0/u_resolution;

#if defined(BUFFER_0)
    color = texture2D(u_buffer1, st);

    vec2 mouse = u_mouse/u_resolution;
    float sdf = distance(st, mouse); 
    float padding = 0.025;
    color += (mouse.x > padding && mouse.y > padding && mouse.x < (1.0-padding) && mouse.y < (1.0-padding) ) ? step(sdf, 0.02) : 0.0 ;

#elif defined(BUFFER_1)
    color = texture2D(u_buffer0, st);

#elif defined(CONVOLUTION_PYRAMID_0)
    color.rgb = laplacian(u_tex0, st, pixel, 1.0);

    color *= 1.0-texture2D(u_buffer0, st).r;

    color.rgb = color.xyz * 0.5 + 0.5;

#elif defined(CONVOLUTION_PYRAMID_ALGORITHM)
    if (!u_convolutionPyramidUpscaling) {
        for (int dy = -2; dy <= 2; dy++) {
            for (int dx = -2; dx <= 2; dx++) {
                vec2 uv = st + vec2(float(dx), float(dy)) * pixel * 0.5;
                if (uv.x <= 0.0 || uv.x >= 1.0 || uv.y <= 0.0 || uv.y >= 1.0)
                    continue;

                vec4 sample = texture2D(u_convolutionPyramidTex0, saturate(uv)) ;
                sample.xyz = sample.rgb * 2.0 - 1.0;
                color += sample * h1[ absi(dx) ] * h1[ absi(dy) ];
            }
        }
    }
    else {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                vec2 uv = st + vec2(float(dx), float(dy)) * pixel * 0.5;

                vec4 sample = texture2D(u_convolutionPyramidTex0, uv);
                sample.xyz = sample.rgb * 2.0 - 1.0;
                color += sample * g[ absi(dx) ] * g[ absi(dy) ];
            }
        }

        for (int dy = -2; dy <= 2; dy++) {
            for (int dx = -2; dx <= 2; dx++) {
                vec2 uv = st + vec2(float(dx), float(dy)) * pixel * 2.0;
                if (uv.x <= 0.0 || uv.x >= 1.0 || uv.y <= 0.0 || uv.y >= 1.0)
                    continue;

                vec4 sample = texture2D(u_convolutionPyramidTex1, uv);
                sample.xyz = sample.rgb * 2.0 - 1.0;
                color += sample * h2 * h1[ absi(dx) ] * h1[ absi(dy) ];
            }
        }
    }

    color.rgb = color.xyz * 0.5 + 0.5;
#else
    color.rgb = (texture2D(u_convolutionPyramid0, st).rgb - 0.5) * 4.0;

#endif

    gl_FragColor = color;
}

// Usefull functions

bool isOutside(vec2 pos) {
    return (pos.x < 0.0 || pos.y < 0.0 || pos.x > 1.0 || pos.y > 1.0);
}

vec3 laplacian(sampler2D tex, vec2 st, vec2 pixel, float pixelpad) {
    vec3 color = vec3(0.0);
    vec2 uv = st * vec2(1.0 + pixelpad * 2.0 * pixel) - pixelpad * pixel;

    vec3 pixelShift = vec3(pixel, 0.0);
    if (!isOutside(uv)) color.xyz = 4.0 * texture2D(u_tex0, uv).rgb;
    vec2 uv110 = uv + pixelShift.xz;
    if (!isOutside(uv110)) color.xyz -= texture2D(u_tex0, uv110).rgb;
    vec2 uv101 = uv + pixelShift.zy;
    if (!isOutside(uv101)) color.xyz -= texture2D(u_tex0, uv101).rgb;
    vec2 uv010 = uv - pixelShift.xz;
    if (!isOutside(uv010)) color.xyz -= texture2D(u_tex0, uv010).rgb;
    vec2 uv001 = uv - pixelShift.zy;
    if (!isOutside(uv001)) color.xyz -= texture2D(u_tex0, uv001).rgb;
    return color;
}