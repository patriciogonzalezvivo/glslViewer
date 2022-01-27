#ifdef GL_ES
precision highp float;
#endif

// Pseudo fluids, Made by Keijiro Takahashi (@_kzr), and 
// heavily based on Florian Berger's (@flockaroo) single-pass CFD
// 
// https://www.shadertoy.com/view/MdKXRy

uniform sampler2D   u_doubleBuffer0;
uniform sampler2D   u_tex0;
uniform vec2        u_resolution;
uniform float       u_time;

#define TAU 6.2831853071795864769252867665590
#define ANGLES 5

vec2 cos_sin(float x) { return vec2(cos(x), sin(x)); }
vec2 rot90(vec2 v) { return v.yx * vec2(1.0, -1.0); }

float influence(vec2 uv, float sc) {
    float acc = 0.0;
    float angles = float(ANGLES);
    for (int i = 0; i < ANGLES; i++) {
        vec2 duv = cos_sin(TAU / angles * float(i)) * sc;
        acc += dot(duv, texture2D(u_doubleBuffer0, uv + duv).xy - 0.5);
    }
    return acc / (sc * sc * angles);
}

void main(void) {
    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    vec2 st = gl_FragCoord.xy/u_resolution;

#ifdef DOUBLE_BUFFER_0

    const float delta = 1.0 / 180.0;
    vec2 acc = vec2(0.0);
    float sc = delta;

    for(int i = 0; i < 5; i++) {
        for(int j = 0; j < ANGLES; j++) {
            vec2 duv = cos_sin(TAU / float(ANGLES) * float(j)) * sc;
            acc += rot90(duv) * influence(st + duv, sc);
        }
        sc *= 2.0;
    }

    color = texture2D(u_doubleBuffer0, st + acc * delta);

#else

    // Main Pass
    color = texture2D(u_doubleBuffer0, st);
#endif


    if (mod(u_time, 10.) < 1.) color = texture2D(u_tex0, st);
    gl_FragColor = color;
}