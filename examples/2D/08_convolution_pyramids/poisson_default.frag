
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_tex0;
uniform vec2        u_tex0Resolution;
uniform sampler2D   u_convolutionPyramids;

uniform vec2        u_resolution;

varying vec2        v_texcoord;

void main (void) {
    vec4 color = vec4(0.0);
    vec2 st = v_texcoord;

#if defined(CONVOLUTION_PYRAMID)
    color = texture2D(u_tex0, st);

#else
    color = texture2D(u_convolutionPyramids, st);

#endif

    gl_FragColor = color;
}
