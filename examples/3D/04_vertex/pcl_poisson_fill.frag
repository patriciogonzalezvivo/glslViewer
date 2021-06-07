

#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_scene;
uniform sampler2D   u_convolutionPyramid;

uniform vec2        u_resolution;

varying vec4        v_color;
varying vec2        v_texcoord;

void main(void) {
    vec4 color = vec4(0.0);

#if defined(CONVOLUTION_PYRAMID)
    color = texture2D(u_scene, v_texcoord);

#elif defined(POSTPROCESSING)
    color = texture2D(u_convolutionPyramid, v_texcoord);

#else
    color = v_color;
#endif

    gl_FragColor = color;
}

