#ifdef GL_ES
precision mediump float;
#endif


// Copyright Patricio Gonzalez Vivo, 2021 - http://patriciogonzalezvivo.com/
// I am the sole copyright owner of this Work.
//
// You cannot host, display, distribute or share this Work in any form,
// including physical and digital. You cannot use this Work in any
// commercial or non-commercial product, website or project. You cannot
// sell this Work and you cannot mint an NFTs of it.
// I share this Work for educational purposes, and you can link to it,
// through an URL, proper attribution and unmodified screenshot, as part
// of your educational material. If these conditions are too restrictive
// please contact me and we'll definitely work it out.

//  This examples have a lot of smart tricks are taken from Jaume Sanchez Elias (@thespite)
//  Polygon Shredder https://www.clicktorelease.com/code/polygon-shredder/
//

// position
uniform sampler2D   u_doubleBuffer0; // 512x512

// velocity
uniform sampler2D   u_doubleBuffer1; // 512x512

uniform vec2        u_resolution;
uniform float       u_time;
uniform float       u_delta;
uniform int         u_frame;

varying vec4        v_color;
varying vec2        v_texcoord;

float   random(vec2 st);
vec3    random3(vec2 st);
vec3    random3(vec3 p);
vec3    snoise3( vec3 x );
vec3    curlNoise(vec3 p);
mat4    rotationMatrix(vec3 axis, float angle);
#define decimation(value, presicion) (floor(value * presicion)/presicion)

void main(void) {
    vec4 color = vec4(0.0);
    vec2 pixel = 1.0/ u_resolution;
    vec2 st = gl_FragCoord.xy * pixel;
    vec2 uv = v_texcoord;

    vec4  buff0 = texture2D(u_doubleBuffer0, uv);
    vec4  buff1 = texture2D(u_doubleBuffer1, uv);
    vec3  pos = buff0.xyz;// * 2.0 - 1.0;
    vec3  vel = buff1.xyz;// * 2.0 - 1.0;
    float life = buff0.a * 100.0;

    pos = u_frame < 1 ? random3(st) * 0.01 : pos;
    vel = u_frame < 1 ? curlNoise(vec3(st,0.5)) : vel;
    life = u_frame < 1 ? uv.x  * 10. + uv.y * 90. : life;

#if defined(DOUBLE_BUFFER_0)
    pos += vel;
    life -= 0.003;

    if ( length( pos ) > .75 || life <= 0.0)
        pos = random3(pos + u_time);

    if (life <= 0.0) {
        // pos = pos * 0.1;
        life = 100.0;
    }

    // pos = pos * 0.5 + 0.5;
    color.rgb = pos;
    color.a = life * 0.01;

#elif defined(DOUBLE_BUFFER_1)
    vel *= 0.5;
    vel += curlNoise( pos + u_time * 0.1) * 0.3;

    float dist = length( pos );

    // Repulsion from the very center of the space
    vel += normalize(pos) * 0.5 * (1.0 - dist) * step(dist, 0.01);

    // Atraction to the center of the space conform the leave
    vel += -normalize(pos) * 0.5 * pow(dist, 2.0);
    
    color.rgb = clamp(vel * u_delta, -0.999, 0.999);// * 0.5 + 0.5;
    color.a = 1.0;

#else
    color = v_color;
#endif

    gl_FragColor = color;
}


// USEFULL FUNCTIONS

float random(in vec2 st) {
  return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 random3(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * vec3(0.3,0.6,0.2));
    p3 += dot(p3, p3.yxz+19.19);
    return fract((p3.xxy+p3.yzz)*p3.zyx);
}

vec3 random3(in vec3 p) {
    p = vec3( dot(p, vec3(12.1, 31.7, 74.7)),
            dot(p, vec3(26.5, 18.3, 24.1)),
            dot(p, vec3(13.5, 27.9, 14.6)));
    return -1. + 2. * fract(sin(p) * 43758.5453123);
}

vec3 mod289(in vec3 x) {
  return x - floor(x * (1. / 289.)) * 289.;
}

vec4 mod289(in vec4 x) {
  return x - floor(x * (1. / 289.)) * 289.;
}

vec4 permute(in vec4 x) {
     return mod289(((x * 34.) + 1.)*x);
}

vec4 taylorInvSqrt(in vec4 r) {
  return 1.79284291400159 - 0.85373472095314 * r;
}

float snoise(in vec3 v) {
    const vec2  C = vec2(1.0/6.0, 1.0/3.0) ;
    const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);

    // First corner
    vec3 i  = floor(v + dot(v, C.yyy) );
    vec3 x0 =   v - i + dot(i, C.xxx) ;

    // Other corners
    vec3 g = step(x0.yzx, x0.xyz);
    vec3 l = 1.0 - g;
    vec3 i1 = min( g.xyz, l.zxy );
    vec3 i2 = max( g.xyz, l.zxy );

    //   x0 = x0 - 0.0 + 0.0 * C.xxx;
    //   x1 = x0 - i1  + 1.0 * C.xxx;
    //   x2 = x0 - i2  + 2.0 * C.xxx;
    //   x3 = x0 - 1.0 + 3.0 * C.xxx;
    vec3 x1 = x0 - i1 + C.xxx;
    vec3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y
    vec3 x3 = x0 - D.yyy;      // -1.0+3.0*C.x = -0.5 = -D.y

    // Permutations
    i = mod289(i);
    vec4 p = permute( permute( permute(
                i.z + vec4(0.0, i1.z, i2.z, 1.0 ))
            + i.y + vec4(0.0, i1.y, i2.y, 1.0 ))
            + i.x + vec4(0.0, i1.x, i2.x, 1.0 ));

    // Gradients: 7x7 points over a square, mapped onto an octahedron.
    // The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
    float n_ = 0.142857142857; // 1.0/7.0
    vec3  ns = n_ * D.wyz - D.xzx;

    vec4 j = p - 49.0 * floor(p * ns.z * ns.z);  //  mod(p,7*7)

    vec4 x_ = floor(j * ns.z);
    vec4 y_ = floor(j - 7.0 * x_ );    // mod(j,N)

    vec4 x = x_ *ns.x + ns.yyyy;
    vec4 y = y_ *ns.x + ns.yyyy;
    vec4 h = 1.0 - abs(x) - abs(y);

    vec4 b0 = vec4( x.xy, y.xy );
    vec4 b1 = vec4( x.zw, y.zw );

    //vec4 s0 = vec4(lessThan(b0,0.0))*2.0 - 1.0;
    //vec4 s1 = vec4(lessThan(b1,0.0))*2.0 - 1.0;
    vec4 s0 = floor(b0)*2.0 + 1.0;
    vec4 s1 = floor(b1)*2.0 + 1.0;
    vec4 sh = -step(h, vec4(0.0));

    vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
    vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;

    vec3 p0 = vec3(a0.xy,h.x);
    vec3 p1 = vec3(a0.zw,h.y);
    vec3 p2 = vec3(a1.xy,h.z);
    vec3 p3 = vec3(a1.zw,h.w);

    //Normalise gradients
    vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
    p0 *= norm.x;
    p1 *= norm.y;
    p2 *= norm.z;
    p3 *= norm.w;

    // Mix final noise value
    vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
    m = m * m;
    return 42.0 * dot( m*m, vec4( dot(p0,x0), dot(p1,x1),
                                dot(p2,x2), dot(p3,x3) ) );
}

vec3 snoise3( vec3 x ) {
    float s  = snoise(vec3( x ));
    float s1 = snoise(vec3( x.y - 19.1 , x.z + 33.4 , x.x + 47.2 ));
    float s2 = snoise(vec3( x.z + 74.2 , x.x - 124.5 , x.y + 99.4 ));
    vec3 c = vec3( s , s1 , s2 );
    return c;
}

vec3 curlNoise( vec3 p ){
    const float e = .1;
    vec3 dx = vec3( e   , 0.0 , 0.0 );
    vec3 dy = vec3( 0.0 , e   , 0.0 );
    vec3 dz = vec3( 0.0 , 0.0 , e   );

    vec3 p_x0 = snoise3( p - dx );
    vec3 p_x1 = snoise3( p + dx );
    vec3 p_y0 = snoise3( p - dy );
    vec3 p_y1 = snoise3( p + dy );
    vec3 p_z0 = snoise3( p - dz );
    vec3 p_z1 = snoise3( p + dz );

    float x = p_y1.z - p_y0.z - p_z1.y + p_z0.y;
    float y = p_z1.x - p_z0.x - p_x1.z + p_x0.z;
    float z = p_x1.y - p_x0.y - p_y1.x + p_y0.x;

    const float divisor = 1.0 / ( 2.0 * e );
    return normalize( vec3( x , y , z ) * divisor );
}

mat4 rotationMatrix(vec3 axis, float angle) {

    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;

    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}