#version 140

#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_buffer0;

uniform vec2        u_mouse;
uniform vec2        u_resolution;
uniform float       u_time;

out     vec4        fragColor;

void main (void) {
    vec3 color = vec3(0.0);
    vec2 st = gl_FragCoord.xy/u_resolution.xy;

#if defined(BUFFER_0)
    color = vec3(st.x, st.y, (1.0+sin(u_time))*0.5);
#else 
    color = texture2D(u_buffer0, st).rgb;
#endif

    fragColor = vec4(color,1.0);
}
