

#ifdef GL_ES
precision mediump float;
#endif

varying vec4        v_color;

void main(void) {
    vec4 color = vec4(0.0);

    color = v_color;

    gl_FragColor = color;
}

