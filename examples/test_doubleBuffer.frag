
// Copyright Patricio Gonzalez Vivo, 2016 - http://patriciogonzalezvivo.com/
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

#ifdef GL_ES
precision highp float;
#endif

uniform sampler2D   u_doubleBuffer0;

uniform vec2        u_resolution;
uniform vec2        u_mouse;
uniform float       u_time;
uniform int         u_frame;

#include "lygia/generative/random.glsl"
#include "lygia/draw/stroke.glsl"
#include "lygia/sdf/circleSDF.glsl"

void main() {
    vec3 color = vec3(0.0);
    vec2 st = gl_FragCoord.xy/u_resolution;

#ifdef DOUBLE_BUFFER_0

    color = texture2D(u_doubleBuffer0, st).rgb;

    float d = 0.0;
    d = 1.75 * stroke(circleSDF(st - u_mouse/u_resolution + 0.5), 0.05, 0.01) * random(st + u_time);

    //  Grab the information arround the active pixel
    //
   	float s0 = color.y;
   	vec3  pixel = vec3(vec2(2.0)/u_resolution.xy,0.);
    float s1 = texture2D(u_doubleBuffer0, st + (-pixel.zy)).r;    //     s1
    float s2 = texture2D(u_doubleBuffer0, st + (-pixel.xz)).r;    //  s2 s0 s3
    float s3 = texture2D(u_doubleBuffer0, st + (pixel.xz)).r;     //     s4
    float s4 = texture2D(u_doubleBuffer0, st + (pixel.zy)).r;
    d += -(s0 - .5) * 2. + (s1 + s2 + s3 + s4 - 2.);

    d *= 0.99;
    d *= (u_frame <= 1)? 0.0 : 1.0; // Clean buffer at startup
    d = clamp(d * 0.5 + 0.5, 0.0, 1.0);

    color = vec3(d, color.x, 0.0);

#else
    // Main Buffer
    color = texture2D(u_doubleBuffer0, st).rgb;

#endif

    gl_FragColor = vec4(color, 1.0);
}
