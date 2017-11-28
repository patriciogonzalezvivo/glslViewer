#ifdef GL_ES
precision mediump float;
#endif

#define ITERATIONS 9

uniform sampler2D u_backbuffer;
uniform vec2 u_resolution;
uniform vec2 u_mouse;
uniform float u_time;

float diffU = 0.25;
float diffV = 0.05;
float f = 0.1;
float k = 0.063;

float random (in float x) {
    return fract(sin(x)*43758.5453123);
}

float random (vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233)))*43758.5453123);
}

void main(void){
    vec2 st   = gl_FragCoord.st/u_resolution;
    vec2 pixel = 1./u_resolution;

    float kernel[9];
    kernel[0] = 0.707106781;
    kernel[1] = 1.0;
    kernel[2] = 0.707106781;
    kernel[3] = 1.0;
    kernel[4] = -6.82842712;
    kernel[5] = 1.0;
    kernel[6] = 0.707106781;
    kernel[7] = 1.0;
    kernel[8] = 0.707106781;

    vec2 offset[9];
    offset[0] = pixel * vec2(-1.0,-1.0);
    offset[1] = pixel * vec2( 0.0,-1.0);
    offset[2] = pixel * vec2( 1.0,-1.0);

    offset[3] = pixel * vec2(-1.0,0.0);
    offset[4] = pixel * vec2( 0.0,0.0);
    offset[5] = pixel * vec2( 1.0,0.0);

    offset[6] = pixel * vec2(-1.0,1.0);
    offset[7] = pixel * vec2( 0.0,1.0);
    offset[8] = pixel * vec2( 1.0,1.0);

    vec2 texColor = texture2D(u_backbuffer, st).rb;

    vec2 uv = st;
    uv -= u_mouse/u_resolution;
    float pct = random(u_time);
    float srcTexColor = smoothstep(.999+pct*0.0001,1.,1.-dot(uv,uv))*random(st)*pct;

    vec2 lap = vec2(0.0);

    for (int i=0; i < ITERATIONS; i++){
        vec2 tmp = texture2D(u_backbuffer, st + offset[i]).rb;
        lap += tmp * kernel[i];
    }

    float F  = f + srcTexColor * 0.025 - 0.0005;
    float K  = k + srcTexColor * 0.025 - 0.0005;

    float u  = texColor.r;
    float v  = texColor.g + srcTexColor * 0.5;

    float uvv = u * v * v;

    float du = diffU * lap.r - uvv + F * (1.0 - u);
    float dv = diffV * lap.g + uvv - (F + K) * v;

    u += du * 0.6;
    v += dv * 0.6;

    gl_FragColor = vec4(clamp( u, 0.0, 1.0 ), 1.0 - u/v ,clamp( v, 0.0, 1.0 ), 1.0);
 }
