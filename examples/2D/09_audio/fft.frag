#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_tex0;
uniform vec2        u_tex0Resolution;

uniform vec2        u_resolution;

#define decimation(value, presicion) (floor(value * presicion)/presicion)

vec3 heatmap(float v) {
    vec3 r = v * 2.1 - vec3(1.8, 1.14, 0.3);
    return 1.0 - r * r;
}

void main (void) {
    vec3 color = vec3(0.0);
    vec2 st = gl_FragCoord.xy/u_resolution.xy;
    float half_bar = 1./u_tex0Resolution.x;

    float bars = 20.;
    float bar = decimation(st.x, bars);
    vec4 value = texture2D(u_tex0, vec2(bar + half_bar, 0.5) );

    color += heatmap(bar) * step(st.y, value.x);


    gl_FragColor = vec4(color,1.0);
}
