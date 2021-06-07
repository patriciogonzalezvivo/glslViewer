
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_tex0;
uniform sampler2D   u_convolutionPyramid;

uniform vec2        u_resolution;
uniform vec2        u_mouse;

vec3 laplace(sampler2D tex, vec2 st, vec2 pixel) {
    return texture2D(tex, st).rgb * 4.0
            - texture2D(tex, st + vec2(-1.0,  0.0) * pixel).rgb
            - texture2D(tex, st + vec2( 0.0, -1.0) * pixel).rgb
            - texture2D(tex, st + vec2( 0.0,  1.0) * pixel).rgb
            - texture2D(tex, st + vec2( 1.0,  0.0) * pixel).rgb;
}

void main (void) {
    vec4 color = vec4(0.0);
    vec2 st = gl_FragCoord.xy/u_resolution;
    vec2 pixel = 1.0/u_resolution;

#if defined(CONVOLUTION_PYRAMID)
    color.rgb = laplace(u_tex0, st, pixel * 4.0);
    color.a = step(.1, dot(color.rgb, color.rgb) );

#else
    color = texture2D(u_convolutionPyramid, st); 
    color = mix(color, texture2D(u_tex0, st), step(st.x, u_mouse.x/u_resolution.x));

#endif

    gl_FragColor = color;
}
