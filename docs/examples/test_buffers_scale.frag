#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_tex0;
uniform vec2        u_tex0Resolution;

uniform sampler2D   u_buffer0; // 0.125

uniform vec2        u_mouse;
uniform vec2        u_resolution;
uniform float       u_time;

varying vec2        v_texcoord;

void main (void) {
    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    vec2 st = gl_FragCoord.xy/u_resolution.xy;
    vec2 uv = v_texcoord;

#if defined(BUFFER_0)
    color = texture2D(u_tex0, uv);

#else
    color = texture2D(u_buffer0, st);

#endif

    gl_FragColor = color;
}
