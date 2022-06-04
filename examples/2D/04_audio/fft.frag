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

float stroke(float x, float size, float w) {
    return step(size, x + w * 0.5) - step(size, x - w * 0.5);
}

void main (void) {
    vec3 color = vec3(0.0);
    vec2 pixel = 1.0/u_resolution.xy;
    vec2 st = gl_FragCoord.xy * pixel;
    float half_bar = 1./u_tex0Resolution.x;

    // fft
    float bar = decimation(st.x, 20.);
    float fft = texture2D(u_tex0, vec2(bar + half_bar, 0.5) ).r;
    color += heatmap(bar) * step(st.y, fft);

    // amp
    float amp = texture2D(u_tex0, vec2(st.x, 0.5) ).g;
    color += stroke(st.y, amp * 0.5 + 0.5, pixel.y);

    // hist
    vec2 grid = vec2(u_resolution.x, 2.0);
    float his = texture2D(u_tex0, vec2(decimation(st.x, grid.x) + half_bar, 0.5) ).b;
    vec2 ipos = floor(st*grid);
    vec2 fpos = fract(st*grid);
    fpos.y = (mod(ipos.y,2.) == 0.)? 1.0-fpos.y : fpos.y;
    color += step(fpos.y, pow(his, 10.) * 10. );
    
    gl_FragColor = vec4(color,1.0);
}
