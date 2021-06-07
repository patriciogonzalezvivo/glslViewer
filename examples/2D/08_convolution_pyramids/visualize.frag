
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_tex0;
uniform vec2        u_tex0Resolution;

uniform sampler2D   u_convolutionPyramid;
uniform sampler2D   u_convolutionPyramidTex0;
uniform sampler2D   u_convolutionPyramidTex1;
uniform int         u_convolutionPyramidDepths;
uniform int         u_convolutionPyramidDepth;
uniform bool        u_convolutionPyramidUpscaling;

uniform vec2        u_resolution;
uniform int         u_frame;
uniform float       u_time;


vec3 heatmap(float v) {
    vec3 r = v * 2.1 - vec3(1.8, 1.14, 0.3);
    return 1.0 - r * r;
}

float rectSDF(in vec2 st, in vec2 s) {
    st = st * 2. - 1.;
    return max( abs(st.x / s.x),
                abs(st.y / s.y) );
}

const vec3  h1      = vec3(1.0334, 0.6836, 0.1507);
const float h2      = 0.0270;
const vec2  g       = vec2(0.7753, 0.0312);

#define saturate(x) clamp(x, 0.0, 1.0)
#define absi(x)     ( (x < 0)? x * -1 : x )
#define modi(a,b)   ( a - (b * int(a/b)))

void main (void) {
    vec4 color = vec4(0.0);
    vec2 st = gl_FragCoord.xy/u_resolution;
    vec2 pixel = 1.0/u_resolution;
    float sdf = rectSDF(st, vec2(1.0));

#if defined(CONVOLUTION_PYRAMID)
    color = texture2D(u_tex0, st);
    color.rgb *= color.a;

#elif defined(CONVOLUTION_PYRAMID_ALGORITHM)
    if (!u_convolutionPyramidUpscaling) {
        for (int dy = -2; dy <= 2; dy++) {
            for (int dx = -2; dx <= 2; dx++) {
                vec2 uv = st + vec2(float(dx), float(dy)) * pixel;
                if (uv.x <= 0.0 || uv.x >= 1.0 || uv.y <= 0.0 || uv.y >= 1.0)
                    continue;
                vec4 prev = texture2D(u_convolutionPyramidTex0, saturate(uv)) * h1[ absi(dx) ] * h1[ absi(dy) ];

                color.rgb += prev.rgb;
                color.a += prev.a;
            }
        }
    }
    else if (modi(int(u_time),u_convolutionPyramidDepths) == u_convolutionPyramidDepth) {
        color = texture2D(u_convolutionPyramidTex0, st);
    }
    else {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                vec2 uv = st + vec2(float(dx), float(dy)) * pixel;
                color += texture2D(u_convolutionPyramidTex0, saturate(uv)) * g[ absi(dx) ] * g[ absi(dy) ];
            }
        }

        for (int dy = -2; dy <= 2; dy++) {
            for (int dx = -2; dx <= 2; dx++) {
                vec2 uv = st + vec2(float(dx), float(dy)) * pixel * 2.;
                color += texture2D(u_convolutionPyramidTex1, saturate(uv)) * h2 * h1[ absi(dx) ] * h1[ absi(dy) ];
            }
        }
    }
    color = (color.a == 0.0)? color : vec4(color.rgb/color.a, 1.0);

#else
    color = texture2D(u_convolutionPyramid, st);

#endif

    gl_FragColor = color;
}
