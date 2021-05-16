
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_tex0;
uniform vec2        u_tex0Resolution;
uniform sampler2D   u_tex0Depth;
uniform vec2        u_tex0DepthResolution;

uniform vec2        u_resolution;
uniform float       u_time;

// From https://github.com/patriciogonzalezvivo/lygia
vec3 normalMap(sampler2D tex, vec2 st, vec2 pixel) {
    float center      = texture2D(tex, st).r;
    float topLeft     = texture2D(tex, st - pixel).r;
    float left        = texture2D(tex, st - vec2(pixel.x, .0)).r;
    float bottomLeft  = texture2D(tex, st + vec2(-pixel.x, pixel.y)).r;
    float top         = texture2D(tex, st - vec2(.0, pixel.y)).r;
    float bottom      = texture2D(tex, st + vec2(.0, pixel.y)).r;
    float topRight    = texture2D(tex, st + vec2(pixel.x, -pixel.y)).r;
    float right       = texture2D(tex, st + vec2(pixel.x, .0)).r;
    float bottomRight = texture2D(tex, st + pixel).r;
    
    float dX = topRight + 2. * right + bottomRight - topLeft - 2. * left - bottomLeft;
    float dY = bottomLeft + 2. * bottom + bottomRight - topLeft - 2. * top - topRight;

    return normalize(vec3(dX, dY, 0.001) );
}

void main (void) {
    vec3 color = vec3(0.0);
    vec2 st = gl_FragCoord.xy/u_resolution.xy;

    vec2 depth_pixel = 1.0/u_tex0DepthResolution.xy;

    color = texture2D(u_tex0, st).rgb;

    float depth = texture2D(u_tex0Depth, st).r;
    vec3 normal = normalMap(u_tex0Depth, st, depth_pixel);

    float pct = fract( (st.x - st.y) * 0.5 - u_time * 0.1);
    
    color = mix(color, vec3(depth), step(0.33, pct));
    color = mix(color, normal * 0.5 + 0.5, step(0.66, pct));

    gl_FragColor = vec4(color,1.0);
}
