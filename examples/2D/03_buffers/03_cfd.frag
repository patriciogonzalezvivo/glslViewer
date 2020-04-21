// Pseudo fluids, Made by Keijiro Takahashi (@_kzr), and 
// heavily based on Florian Berger's (@flockaroo) single-pass CFD
// 
// https://www.shadertoy.com/view/MdKXRy

uniform sampler2D   u_buffer0;
uniform sampler2D   u_buffer1;
uniform sampler2D   u_tex0;
uniform vec2        u_resolution;
uniform float       u_time;

#ifndef TAU
#define TAU 6.2831853071795864769252867665590
#endif

#define ANGLES 5

vec2 cos_sin(float x) { return vec2(cos(x), sin(x)); }
vec2 rot90(vec2 v) { return v.yx * vec2(1.0, -1.0); }

float influence(vec2 uv, float sc) {
    float acc = 0.0;
    float angles = float(ANGLES);
    for (int i = 0; i < ANGLES; i++) {
        vec2 duv = cos_sin(TAU / angles * float(i)) * sc;
        acc += dot(duv, texture2D(u_buffer1, uv + duv).xy - 0.5);
    }
    return acc / (sc * sc * angles);
}

void main(void) {
    vec2 st = gl_FragCoord.xy/u_resolution;

#ifdef BUFFER_0
    // PING BUFFER
    //
    //  Note: Here is where most of the action happens. But need's to read
    //  te content of the previous pass, for that we are making another buffer
    //  BUFFER_1 (u_buffer1)
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

    gl_FragColor = texture2D(u_buffer1, st + acc * delta);

#elif defined( BUFFER_1 )
    // PONG BUFFER
    //
    //  Note: Just copy the content of the BUFFER0 so it can be 
    //  read by it in the next frame
    //
    gl_FragColor = texture2D(u_buffer0, st);
    if (mod(u_time, 10.) < 1.) gl_FragColor = texture2D(u_tex0, st);
#else

    // Main Buffer
    gl_FragColor = texture2D(u_buffer1, st);
#endif
}