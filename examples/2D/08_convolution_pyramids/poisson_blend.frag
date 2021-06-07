
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_target;
uniform vec2        u_targetResolution;
uniform sampler2D   u_source;
uniform vec2        u_sourceResolution;
uniform sampler2D   u_mask;
uniform vec2        u_maskResolution;

uniform sampler2D   u_buffer0;
uniform sampler2D   u_buffer1;
uniform sampler2D   u_buffer2;
uniform sampler2D   u_buffer3;
uniform sampler2D   u_buffer4;

uniform sampler2D   u_convolutionPyramid;
uniform sampler2D   u_convolutionPyramidTex0;
uniform sampler2D   u_convolutionPyramidTex1;
uniform bool        u_convolutionPyramidUpscaling;

uniform vec2        u_resolution;

varying vec2        v_texcoord;

const vec3  h1      = vec3(1.0334, 0.6836, 0.1507);
const float h2      = 0.0270;
const vec2  g       = vec2(0.7753, 0.0312);

#define saturate(x) clamp(x, 0.0, 1.0)
#define absi(x)     ( (x < 0)? x * -1 : x )

#define NUM_NEIGHBORS 4

void main (void) {
    vec4 color = vec4(0.0,0.0,0.0,1.0);
    vec2 st = v_texcoord;
    vec2 pixel = 1.0/u_resolution;
    float amount = 1.0;

    float mixingGradients;
    vec2 neighbors[NUM_NEIGHBORS];
    neighbors[0] = vec2(-1.0, 0.0) * pixel * amount;
    neighbors[1] = vec2( 1.0, 0.0) * pixel * amount; 
    neighbors[2] = vec2(0.0, -1.0) * pixel * amount; 
    neighbors[3] = vec2(0.0,  1.0) * pixel * amount;

#if defined(BUFFER_0)
    color = texture2D(u_target, ((st-0.5) * vec2(u_targetResolution.y/u_targetResolution.x,1.0)+0.5));

#elif defined(BUFFER_1)
    for (int i = 0; i < NUM_NEIGHBORS; ++i) {
        float amount = 2.0;// + 2.0 * pct;
        vec2 offset = st + neighbors[i] * amount;
        color.rgb += texture2D(u_buffer0, st).rgb - texture2D(u_buffer0, offset).rgb;
    }
    // color.rgb /= float(NUM_NEIGHBORS); 
    color.rgb = color.rgb * 0.5 + 0.5;

#elif defined(BUFFER_2)
    float scale = 2.0;
    vec2 uv = (st-0.5) * vec2(u_sourceResolution.y/u_sourceResolution.x,1.0) * scale + 0.5;
    color.rgb = texture2D(u_source, saturate(uv)).rgb;
    color.a = texture2D(u_mask, saturate(uv)).r;

#elif defined(BUFFER_3)
    for (int i = 0; i < NUM_NEIGHBORS; ++i) {
        float amount = 10.0;
        vec2 offset = st + neighbors[i] * amount;
        color.rgb += texture2D(u_buffer2, st).rgb - texture2D(u_buffer2, offset).rgb;
    }
    color.rgb /= float(NUM_NEIGHBORS); 
    color.rgb = color.rgb * 0.5 + 0.5;

#elif defined(CONVOLUTION_PYRAMID)
    color = texture2D(u_buffer0, st);
    color.a = 1.0-texture2D(u_buffer2, st).a;
    color.rgb *= color.a;

    float msk = texture2D(u_buffer2, st).a;
    vec3 src = texture2D(u_buffer3, st).rgb * 2.0 - 1.0;

    // color.rgb += (src) * msk;
    // color.a += pow(length(src), 10.) * msk;

#elif defined(CONVOLUTION_PYRAMID_ALGORITHM)
    color.a = 0.0;

    if (!u_convolutionPyramidUpscaling) {
        for (int dy = -2; dy <= 2; dy++) {
            for (int dx = -2; dx <= 2; dx++) {
                vec2 uv = st + vec2(float(dx), float(dy)) * pixel;
                if (uv.x <= 0.0 || uv.x >= 1.0 || uv.y <= 0.0 || uv.y >= 1.0)
                    continue;
                vec4 prev = texture2D(u_convolutionPyramidTex0, uv) * h1[ absi(dx) ] * h1[ absi(dy) ];
                color += prev;
            }
        }

        color = (color.a == 0.0)? color : vec4(color.rgb/color.a, 1.0);


    }
    else {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                vec2 uv = st + vec2(float(dx), float(dy)) * pixel;
                vec4 prev = texture2D(u_convolutionPyramidTex0, uv) * g[ absi(dx) ] * g[ absi(dy) ];
                color += prev;
            }
        }

        for (int dy = -2; dy <= 2; dy++) {
            for (int dx = -2; dx <= 2; dx++) {
                vec2 uv = st + vec2(float(dx), float(dy)) * pixel * 2.;
                vec4 prev = texture2D(u_convolutionPyramidTex1, uv) * h2 * h1[ absi(dx) ] * h1[ absi(dy) ];
                color += prev;
            }
        }

        color = (color.a == 0.0)? color : vec4(color.rgb/color.a, 1.0);
    }

#else
    vec4 target = texture2D(u_buffer0, st);
    vec4 source = texture2D(u_buffer2, st);
    
    color = texture2D(u_convolutionPyramid, st);
    float msk = texture2D(u_buffer2, st).a;
    vec3 src = texture2D(u_buffer3, st).rgb * 2.0 - 1.0;
    vec3 baseGrad = texture2D(u_buffer1, st).rgb * 2.0 - 1.0;
    // color.rgb += src * msk;

#endif

    gl_FragColor = color;
}
