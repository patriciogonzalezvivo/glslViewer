#version 120

#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_tex0;
#ifdef STREAMS_PREVS
uniform sampler2D   u_tex0Prev[STREAMS_PREVS];
#endif
uniform vec2        u_tex0Resolution;
uniform float       u_tex0Time;
uniform float       u_tex0Duration;
uniform float       u_tex0CurrentFrame;
uniform float       u_tex0TotalFrames;
uniform float       u_tex0Fps;

uniform vec2        u_resolution;

varying vec2        v_texcoord;

void main (void) {
    vec4 color = vec4(vec3(0.0), 1.0);
    vec2 pixel = 1.0/u_resolution.xy;
    vec2 st = gl_FragCoord.xy * pixel;
    vec2 uv = v_texcoord;

    color = texture2D(u_tex0, st);
    
    #ifdef STREAMS_PREVS
    vec2 uv2 = uv * max(4.0, float(STREAMS_PREVS));
    vec2 uv2i = floor(uv2);
    vec2 uv2f = fract(uv2);

    if (uv2i.y == 0.0 && uv2i.x < float(STREAMS_PREVS) ) {
        int i = int(uv2i.x);
        color = texture2D( u_tex0Prev[i], uv2f);
    }
    #endif

    color.rgb = mix( color.rgb, vec3(step(st.x, u_tex0Time/ u_tex0Duration), 0., 0.), step(st.y, 0.006) );
    color.rgb = mix( color.rgb, vec3(step(st.x, u_tex0CurrentFrame/u_tex0TotalFrames), 0., 0.), step(st.y, 0.009) );
    color.rgb = mix( color.rgb, vec3(step(st.x, fract(u_tex0CurrentFrame)), 0., 0.) , step(st.y, 0.003) );

    gl_FragColor = color;
}
