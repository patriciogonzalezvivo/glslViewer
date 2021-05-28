
#ifdef GL_ES
precision mediump float;
#endif

uniform vec2        u_resolution;

uniform sampler2D   u_tex0;
uniform vec2        u_tex0Resolution;

uniform sampler2D   u_poissonFill;

void main (void) {
    vec4 color = vec4(0.0);
    vec2 st = gl_FragCoord.xy/u_resolution.xy;

#if defined(POISSON_FILL)
    color = texture2D(u_tex0, st);
    // color.a = min(color.r, min(color.g, color.b));

#else
    color = texture2D(u_poissonFill, st);

#endif

    gl_FragColor = color;
}
