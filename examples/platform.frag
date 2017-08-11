#ifdef GL_ES
precision mediump float;
#endif

void main (void) {
    gl_FragColor = vec4(1.0);
    #ifdef PLATFORM_OSX
        gl_FragColor = vec4(1.0,0.0,0.0,1.0);
    #endif

    #ifdef PLATFORM_LINUX
        gl_FragColor = vec4(0.0,1.0,0.0,1.0);
    #endif

    #ifdef PLATFORM_RPI
        gl_FragColor = vec4(0.0,0.0,1.0,1.0);
    #endif
}
