// Created by Sergei B (https://github.com/bespsm)
// Example of basic audio wave and frequency data usage.

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_resolution;
uniform sampler2D u_tex0;

void main (void) {

    vec2 uv = gl_FragCoord.xy/u_resolution.xy;

    // Frequency.
    float fft = texture2D(u_tex0, vec2(uv.x, 0.)).x;

    // Volume.
    float vol = texture2D(u_tex0, vec2(uv.x, 0.)).y;

    vec3 col = vec3(0.0);
    col += 1.0 - smoothstep( 0.0, 0.010, abs(vol - uv.y) );
    col += 1.0 - smoothstep( 0.0, 0.030, abs(fft - uv.y) );

    col = pow( col, vec3(1.0,0.5,2.0) );

    gl_FragColor = vec4(col,1.0);
}
