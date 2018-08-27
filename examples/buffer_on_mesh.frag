#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_buffer0;
uniform sampler2D   u_buffer1;
uniform sampler2D   u_buffer2;

uniform vec3        u_light;
uniform vec2        u_resolution;
uniform vec2        u_mouse;
uniform float       u_time;

varying vec4        v_position;
varying vec3        v_normal;
varying vec2        v_texcoord;

#define ITERATIONS 9

float diffU = 0.295;
float diffV = 0.05;
float f = 0.1;
float k = 0.06;

float random (in float x) {
    return fract(sin(x)*43758.5453123);
}

float random (vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233)))*43758.5453123);
}


#ifndef NORMALMAP_Z
#define NORMALMAP_Z .01
#endif

#ifndef NORMALMAP_SAMPLER_FNC
#define NORMALMAP_SAMPLER_FNC(POS_UV) texture2D(tex, clamp(POS_UV, 0., 1.0)).g
#endif

vec3 normalMap(sampler2D tex, vec2 st, vec2 pixel) {
  float center = NORMALMAP_SAMPLER_FNC(st);
  float topLeft	= NORMALMAP_SAMPLER_FNC(st - pixel);
  float left = NORMALMAP_SAMPLER_FNC(st.x - vec2(pixel.x, .0));
  float bottomLeft = NORMALMAP_SAMPLER_FNC(st + vec2(-pixel.x, pixel.y));
  float top	= NORMALMAP_SAMPLER_FNC(st - vec2(.0, pixel.y));
  float bottom = NORMALMAP_SAMPLER_FNC(st + vec2(.0, pixel.y));
  float topRight = NORMALMAP_SAMPLER_FNC(st + vec2(pixel.x, -pixel.y));
  float right	= NORMALMAP_SAMPLER_FNC(st + vec2(pixel.x, .0));
  float bottomRight = NORMALMAP_SAMPLER_FNC(st + pixel);
  float dX = topRight + 2. * right + bottomRight - topLeft - 2. * left - bottomLeft;
  float dY = bottomLeft + 2. * bottom + bottomRight - topLeft - 2. * top - topRight;
  return normalize(vec3(clamp(dX, -1., 1.),
                        clamp(dY, -1., 1.),
                        NORMALMAP_Z));
}

void main() {
    vec2 st = v_texcoord;
    vec2 pixel = 1./u_resolution;
    // st.y = 1.0 - st.y;

#ifdef BUFFER_0
    // PING BUFFER
    //
    //  Note: Here is where most of the action happens. But need's to read
    //  te content of the previous pass, for that we are making another buffer
    //  BUFFER_1 (u_buffer1)
    
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

    vec2 texColor = texture2D(u_buffer1, st).rb;

    vec2 uv = st;
    float t = u_time;
    uv -= u_mouse/u_resolution;
    float pct = random(u_time);
    float srcTexColor = smoothstep(.999+pct*0.0001,1.,1.-dot(uv,uv))*random(st)*pct;

    vec2 lap = vec2(0.0);

    for (int i=0; i < ITERATIONS; i++){
        vec2 tmp = texture2D(u_buffer1, st + offset[i]).rb;
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

#elif defined( BUFFER_1 )
    // PONG BUFFER
    //
    //  Note: Just copy the content of the BUFFER0 so it can be 
    //  read by it in the next frame
    //
    gl_FragColor = texture2D(u_buffer0, st);

#elif defined( BUFFER_2 )
    // PONG BUFFER
    //
    //  Note: Just copy the content of the BUFFER0 so it can be 
    //  read by it in the next frame
    //

    vec3 normal = normalMap(u_buffer1, st, pixel) * 0.5 + 0.5;
    float depth = 1.1 - texture2D(u_buffer1, st).r;

    gl_FragColor = vec4(normal, depth);

#else
    // Main Buffer
    vec3 color = vec3(0.0);

    color = texture2D(u_buffer1, st).rgb;
    color *= dot(normalize(v_normal), normalize(vec3(u_light))) * 0.95;
    // color.r = 1.;
    
    gl_FragColor = vec4(color, 1.0);
#endif
}