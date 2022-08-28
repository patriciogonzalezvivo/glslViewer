#ifdef GL_ES
precision highp float;
#endif

// Copyright Patricio Gonzalez Vivo, 2021 - http://patriciogonzalezvivo.com/
// I am the sole copyright owner of this Work.
//
// You cannot host, display, distribute or share this Work in any form,
// including physical and digital. You cannot use this Work in any
// commercial or non-commercial product, website or project. You cannot
// sell this Work and you cannot mint an NFTs of it.
// I share this Work for educational purposes, and you can link to it,
// through an URL, proper attribution and unmodified screenshot, as part
// of your educational material. If these conditions are too restrictive
// please contact me and we'll definitely work it out.

// Based on the work of: 
//  - Sage Jenson @mxsage https://sagejenson.com/physarum
//  - Ken Voskuil @kaesve https://kaesve.nl/projects/mold/summary.html
//  - Domenicobrz @Domenico_brz https://github.com/Domenicobrz/Physarum-experiments

uniform sampler2D   u_scene;

// position / direction PING-PONG
uniform sampler2D   u_doubleBuffer0; // 512x512

// density PING-PONG
uniform sampler2D   u_doubleBuffer1;

uniform vec2        u_resolution;
uniform float       u_time;
uniform float       u_delta;
uniform int         u_frame;

varying vec4        v_color;
varying vec2        v_texcoord;

#define TAU 6.2831853071795864769252867665590
float   maxSpeed = 0.5;
float   sensorAngle = radians(25.0);
float   sensorDistance = 17.5;
float   decayRate = 0.9875;

vec2    rotate(vec2 v, float a);
float   random(vec2 st);
vec2    random2(vec2 p);
float   getTrailValue(vec2 pos);
float   getDiffuseTrailValue(vec2 pos, vec2 pixel);
#define decimation(value, presicion) (floor(value * presicion)/presicion)

void main(void) {
    vec4 color = vec4(vec3(0.0), 1.0);
    vec2 pixel = 1.0/u_resolution;
    vec2 st = gl_FragCoord.xy * pixel;
    vec2 uv = v_texcoord;

    vec2 buffRes = vec2(512.0) * 2.0;
    vec2 buffPixel = 1.0/buffRes;
    vec2 data_uv = uv;//decimation(st, buffRes) + 0.5 * pixel;
    vec4 data = texture2D(u_doubleBuffer0, data_uv);

// PARTICLES PINGPONG
#if defined(DOUBLE_BUFFER_0)

    // Initial parameters
    vec2 pos = data.xy;
    pos = u_frame < 1 ? random2(st) * 0.2 - 0.01: pos;

    vec2 dir = data.zw;
    dir = u_frame < 1 ? random2(st) * 2.0 - 1.0: dir;

    float speed = 0.01 + random(st) * maxSpeed;

    vec2 pos_uv = fract(pos * 0.5 + 0.5);
    vec2 lDir = normalize(  rotate(dir, +sensorAngle)  );
    vec2 cDir = normalize(  dir  );
    vec2 rDir = normalize(  rotate(dir, -sensorAngle)  );
    float lTrailValue = getTrailValue( pos_uv + buffPixel * sensorDistance * lDir);
    float cTrailValue = getTrailValue( pos_uv + buffPixel * sensorDistance * cDir);
    float rTrailValue = getTrailValue( pos_uv + buffPixel * sensorDistance * rDir);

    float highestValue = cTrailValue;
    vec2 newDir = cDir;

    if (lTrailValue > cTrailValue) {
        newDir = lDir;
        highestValue = lTrailValue;
    }
    else if (rTrailValue > highestValue)
        newDir = rDir;

    else if (rTrailValue == lTrailValue)
        newDir = (random(st + u_time) < 0.5) ? lDir : rDir;

    pos += speed * newDir * u_delta;

    if ( abs(pos.x) >= 0.95 || abs(pos.y) >= 0.95 )
        pos = random2(st + pos) * 2.0 - 1.0;

    color = vec4(pos, newDir);

// TRAIL PINGPONG
#elif defined(DOUBLE_BUFFER_1)
    color.rgb += texture2D(u_scene, st).r;
    color.rgb += getDiffuseTrailValue(st, pixel) * decayRate;

#elif defined(POSTPROCESSING)
    color.rgb = texture2D(u_scene, st).rgb * 0.5;
    color.rgb *= 0.5 + texture2D(u_doubleBuffer1, st).r;

#else
    color = v_color;
#endif

    gl_FragColor = color;
}


// USEFULL FUNCTIONS
//
float random(in vec2 st) {
  return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

vec2 random2(vec2 p) {
    return fract(vec2(p.xy) * vec2(12.9898, 78.233) * 32.643);
}

float getTrailValue(vec2 uv) {
    return texture2D(u_doubleBuffer1, clamp(uv, 0.0, 1.0) ).r;
}

float getDiffuseTrailValue(vec2 st, vec2 pixel) {
    vec3 spacing = vec3(pixel, 0.0);
    return (getTrailValue(st) + (
        (
            getTrailValue(st + spacing.zy) + // north
            getTrailValue(st + spacing.xz) + // east
            getTrailValue(st - spacing.zy) + // south
            getTrailValue(st - spacing.xz)   // west
        ) * 0.2 + 
        (
            getTrailValue(st + spacing.zy + spacing.xz) + // ne
            getTrailValue(st - spacing.zy + spacing.xz) + // se
            getTrailValue(st - spacing.zy - spacing.xz) + // sw
            getTrailValue(st + spacing.zy - spacing.xz)   // nw
        ) * 0.05 // 4/5 + 4/20 = (16 + 4)/20 = 1
        )
    ) * 0.5;
}

vec2 rotate(vec2 v, float a) {
    float s = sin(a);
    float c = cos(a);
    return mat2(c, -s, s, c) * v;
}