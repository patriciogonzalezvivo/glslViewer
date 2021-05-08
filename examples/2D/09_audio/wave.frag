#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_tex0;
uniform vec2        u_tex0Resolution;

uniform sampler2D   u_buffer0; 
uniform sampler2D   u_buffer1; 

uniform vec2        u_resolution;

#define decimation(value, presicion) (floor(value * presicion)/presicion)

float stroke(float x, float size, float w) {
    float d = step(size, x + w * 0.5) - step(size, x - w * 0.5);
    // return saturate(d);
    return d;
}

void main() {
    vec3 color = vec3(0.0);
    vec2 st = gl_FragCoord.xy/u_resolution.xy;
    vec2 pixel = 1.0/u_resolution;

    vec2 data = texture2D(u_tex0, vec2(st.x, 0.5) ).xy;

#if defined(BUFFER_0)
    color = texture2D(u_buffer1, st - vec2(1., 0.0) * pixel).rgb;
    
    
    color.rg = mix(color.rg, data.xy, step(st.x, pixel.x));

#elif defined(BUFFER_1)
    color = texture2D(u_buffer0, st).rgb;

#else
    // color = texture2D(u_buffer0, st).rgb;

    st.x *= u_resolution.x/u_resolution.y;
    vec2 grid = vec2(20.0,2.0);

    vec2 ipos = floor(st*grid);
    vec2 fpos = fract(st*grid);

    float value = texture2D(u_buffer0, vec2(ipos.x * pixel.x, 0.1) ).y;
    value = pow(value, 12.) * 100.0;

    fpos.y = (mod(ipos.y,2.) == 0.)? 1.0-fpos.y : fpos.y;
    color.r += stroke(st.y, data.y, 0.01);
    color += step(fpos.y * .25, value)*step(.5,fpos.x);


#endif

    gl_FragColor = vec4(color,1.0);
}