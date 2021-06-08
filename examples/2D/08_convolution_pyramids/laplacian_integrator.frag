
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_tex0;
uniform vec2        u_tex0Resolution;

uniform vec2        u_resolution;
uniform vec2        u_mouse;

uniform sampler2D   u_convolutionPyramid;
uniform sampler2D   u_convolutionPyramidTex0;
uniform sampler2D   u_convolutionPyramidTex1;
uniform bool        u_convolutionPyramidUpscaling;

const vec3  h1      = vec3(.85, 0.5, 0.15);
const float h2      = .2;
const vec2  g       = vec2(0.547, 0.175);

// Laplacian Integrator
// const vec3  h1      = vec3(0.7, 0.5, 0.15);
// const float h2      = 1.0;
// const vec2  g       = vec2(0.547, 0.175);

// Poisson Fill
// const vec3  h1      = vec3(1.0334, 0.6836, 0.1507);
// const float h2      = 0.0270;
// const vec2  g       = vec2(0.7753, 0.0312);

#define saturate(x) clamp(x, 0.0, 1.0)
#define absi(x)     ( (x < 0)? x * -1 : x )

vec3 laplace(sampler2D tex, vec2 st, vec2 pixel) {
    return texture2D(tex, st).rgb * 4.0
            - texture2D(tex, st + vec2(-1.0,  0.0) * pixel).rgb
            - texture2D(tex, st + vec2( 0.0, -1.0) * pixel).rgb
            - texture2D(tex, st + vec2( 0.0,  1.0) * pixel).rgb
            - texture2D(tex, st + vec2( 1.0,  0.0) * pixel).rgb;
}

bool isOutside(vec2 pos) {
    return (pos.x < 0.0 || pos.y < 0.0 || pos.x > 1.0 || pos.y > 1.0);
}

void main (void) {
    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    vec2 st = gl_FragCoord.xy/u_resolution;
    vec2 pixel = 1.0/u_resolution;

#if defined(CONVOLUTION_PYRAMID)
    vec2 padSize = (u_resolution + vec2(2.0)) * pixel;
    
    vec2 uv = st * vec2(padSize) - pixel;

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

    color.rgb = color.xyz * 0.5 + 0.5;

#elif defined(CONVOLUTION_PYRAMID_ALGORITHM)
    // color.a = 0.0;

    if (!u_convolutionPyramidUpscaling) {
        for (int dy = -2; dy <= 2; dy++) {
            for (int dx = -2; dx <= 2; dx++) {
                vec2 uv = st + vec2(float(dx), float(dy)) * pixel;
                if (uv.x <= 0.0 || uv.x >= 1.0 || uv.y <= 0.0 || uv.y >= 1.0)
                    continue;

                vec4 sample = texture2D(u_convolutionPyramidTex0, saturate(uv));
                sample.xyz = sample.rgb * 2.0 - 1.0;
                color += sample * h1[ absi(dx) ] * h1[ absi(dy) ];
            }
        }
    }
    else {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                vec2 uv = st + vec2(float(dx), float(dy)) * pixel;

                vec4 sample = texture2D(u_convolutionPyramidTex0, uv);
                sample.xyz = sample.rgb * 2.0 - 1.0;
                color += sample * g[ absi(dx) ] * g[ absi(dy) ];
            }
        }

        for (int dy = -2; dy <= 2; dy++) {
            for (int dx = -2; dx <= 2; dx++) {
                vec2 uv = st + vec2(float(dx), float(dy)) * pixel;
                vec4 sample = texture2D(u_convolutionPyramidTex1, uv);

                sample.xyz = sample.rgb * 2.0 - 1.0;
                color += sample * h2 * h1[ absi(dx) ] * h1[ absi(dy) ];
            }
        }
    }

    color.rgb = color.xyz * 0.5 + 0.5;
#else
    color.rgb = (texture2D(u_convolutionPyramid, st).rgb * 2.0 - 1.0);

    vec3 edge = laplace(u_tex0, st, pixel);

    color.rgb = mix(edge, color.rgb, step(st.y - st.x, 0.));
    color.rgb = mix(texture2D(u_tex0, st).rgb, color.rgb, step(st.x, u_mouse.x/u_resolution.x));

#endif

    gl_FragColor = color;
}
