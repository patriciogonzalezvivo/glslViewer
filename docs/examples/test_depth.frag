
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_tex0;
uniform vec2        u_tex0Resolution;
uniform sampler2D   u_tex0Depth;
uniform vec2        u_tex0DepthResolution;

uniform vec2        u_resolution;
uniform float       u_time;

void main (void) {
    vec3 color = vec3(0.0);
    vec2 st = gl_FragCoord.xy/u_resolution.xy;

    vec2 depth_pixel = 1.0/u_tex0DepthResolution.xy;

    color = texture2D(u_tex0, st).rgb;

    float depth = texture2D(u_tex0Depth, st).r;

    float pct = fract( (st.x - st.y) * 0.5 - u_time * 0.5);
    
    color = mix(color, vec3(depth), step(0.5, pct));

    gl_FragColor = vec4(color,1.0);
}
