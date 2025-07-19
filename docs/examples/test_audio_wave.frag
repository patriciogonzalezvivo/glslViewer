#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_tex0;
uniform vec2        u_tex0Resolution;

uniform sampler2D   u_buffer0; 
uniform sampler2D   u_buffer1; 

uniform vec2        u_resolution;

#include "../deps/lygia/draw/stroke.glsl"

void main() {
    vec3 color = vec3(0.0);
    vec2 pixel = 1.0/u_resolution;
    vec2 st = gl_FragCoord.xy * pixel;

    float amp = texture2D(u_tex0, vec2(st.x, 0.5) ).y;
    color += stroke(st.y, amp, pixel.x);


    gl_FragColor = vec4(color,1.0);
}