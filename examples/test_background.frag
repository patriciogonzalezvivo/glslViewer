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
precision mediump float;
#endif

uniform vec3    u_light;
uniform vec2    u_resolution;

#ifdef HOLOPLAY
uniform vec4    u_viewport;
#define RESOLUTION u_viewport.zw
#else
#define RESOLUTION u_resolution
#endif

uniform float   u_time;

#ifdef MODEL_VERTEX_COLOR
varying vec4    v_color;
#endif

#ifdef MODEL_VERTEX_NORMAL
varying vec3    v_normal;
#endif

vec2 ratio(in vec2 st, in vec2 s) {
    return mix( vec2((st.x*s.x/s.y)-(s.x*.5-s.y*.5)/s.y,st.y),
                vec2(st.x,st.y*(s.y/s.x)-(s.y*.5-s.x*.5)/s.x),
                step(s.x,s.y));
}

void main(void) {
   vec3 color = vec3(1.0);

   vec2 st = gl_FragCoord.xy/RESOLUTION;

#ifdef BACKGROUND
    st = ratio(st, u_resolution);
    color *= vec3(0.5) * step(0.5, fract((st.x - st.y - u_time * 0.05) * 20.));
#else
    #ifdef MODEL_VERTEX_COLOR
    color *= v_color.rgb;
    #endif

    vec3 n = normalize(v_normal);
    vec3 l = normalize(u_light);
    float diffuse = (dot(n, l) + 1.0 ) * 0.5;

    color *= diffuse;
#endif

    gl_FragColor = vec4(color, 1.0);
}
