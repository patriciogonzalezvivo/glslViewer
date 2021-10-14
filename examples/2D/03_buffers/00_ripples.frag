#ifdef GL_ES
precision highp float;
#endif

uniform sampler2D   u_buffer0;
uniform sampler2D   u_buffer1;

uniform vec2        u_resolution;
uniform vec2        u_mouse;
uniform float       u_time;

float random (in float x) {
    return fract(sin(x)*43758.5453123);
}

float random (vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233)))*43758.5453123);
}

#include "../02_pixelspiritdeck/lib/stroke.glsl"
#include "../02_pixelspiritdeck/lib/circleSDF.glsl"

void main() {
    vec3 color = vec3(0.0);
    vec2 st = gl_FragCoord.xy/u_resolution;

#ifdef BUFFER_0
    // PING BUFFER
    //
    //  Note: Here is where most of the action happens. But need's to read
    //  te content of the previous pass, for that we are making another buffer
    //  BUFFER_1 (u_buffer1)

    color = texture2D(u_buffer1, st).rgb;

    float d = 0.0;
    d = 1.75 * stroke(circleSDF(st - u_mouse/u_resolution + 0.5), 0.05, 0.01) * random(st + u_time);

    //  Grab the information arround the active pixel
    //
   	float s0 = color.y;
   	vec3  pixel = vec3(vec2(2.0)/u_resolution.xy,0.);
    float s1 = texture2D(u_buffer1, st + (-pixel.zy)).r;    //     s1
    float s2 = texture2D(u_buffer1, st + (-pixel.xz)).r;    //  s2 s0 s3
    float s3 = texture2D(u_buffer1, st + (pixel.xz)).r;     //     s4
    float s4 = texture2D(u_buffer1, st + (pixel.zy)).r;
    d += -(s0 - .5) * 2. + (s1 + s2 + s3 + s4 - 2.);

    d *= 0.99;
    d *= smoothstep(0.0, 1.0, step(0.5, u_time)); // Clean buffer at startup
    d = clamp(d * 0.5 + 0.5, 0.0, 1.0);

    color = vec3(d, color.x, 0.0);

#elif defined( BUFFER_1 )
    // PONG BUFFER
    //
    //  Note: Just copy the content of the BUFFER0 so it can be 
    //  read by it in the next frame
    //
    color = texture2D(u_buffer0, st).rgb;

#else
    // Main Buffer
    color = texture2D(u_buffer1, st).rgb;

#endif

    gl_FragColor = vec4(color, 1.0);
}