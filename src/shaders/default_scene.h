#pragma once

// DEFAULT SHADERS
// -----------------------------------------------------
const std::string default_scene_vert = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform mat4    u_modelViewProjectionMatrix;

attribute vec4  a_position;
varying vec4    v_position;

#ifdef MODEL_VERTEX_COLOR
attribute vec4  a_color;
varying vec4    v_color;
#endif

#ifdef MODEL_VERTEX_NORMAL
attribute vec3  a_normal;
varying vec3    v_normal;
#endif

#ifdef MODEL_VERTEX_TEXCOORD
attribute vec2  a_texcoord;
varying vec2    v_texcoord;
#endif

#ifdef MODEL_VERTEX_TANGENT
attribute vec4  a_tangent;
varying vec4    v_tangent;
varying mat3    v_tangentToWorld;
#endif

#ifdef LIGHT_SHADOWMAP
uniform mat4    u_lightMatrix;
varying vec4    v_lightCoord;
#endif

void main(void) {
    
    v_position = a_position;
    
#ifdef MODEL_VERTEX_COLOR
    v_color = a_color;
#endif
    
#ifdef MODEL_VERTEX_NORMAL
    v_normal = a_normal;
#endif
    
#ifdef MODEL_VERTEX_TEXCOORD
    v_texcoord = a_texcoord;
#endif
    
#ifdef MODEL_VERTEX_TANGENT
    v_tangent = a_tangent;
    vec3 worldTangent = a_tangent.xyz;
    vec3 worldBiTangent = cross(v_normal, worldTangent);// * sign(a_tangent.w);
    v_tangentToWorld = mat3(normalize(worldTangent), normalize(worldBiTangent), normalize(v_normal));
#endif
    
#ifdef LIGHT_SHADOWMAP
    v_lightCoord = u_lightMatrix * v_position;
#endif
    
    gl_Position = u_modelViewProjectionMatrix * v_position;
}
)";

const std::string default_scene_vert_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform mat4    u_modelViewProjectionMatrix;

in      vec4    a_position;
out     vec4    v_position;

#ifdef MODEL_VERTEX_COLOR
in      vec4    a_color;
out     vec4    v_color;
#endif

#ifdef MODEL_VERTEX_NORMAL
in      vec3    a_normal;
out     vec3    v_normal;
#endif

#ifdef MODEL_VERTEX_TEXCOORD
in      vec2    a_texcoord;
out     vec2    v_texcoord;
#endif

#ifdef MODEL_VERTEX_TANGENT
in      vec4    a_tangent;
out     vec4    v_tangent;
out     mat3    v_tangentToWorld;
#endif

#ifdef LIGHT_SHADOWMAP
uniform mat4    u_lightMatrix;
out     vec4    v_lightCoord;
#endif

void main(void) {
    v_position = a_position;
    
#ifdef MODEL_VERTEX_COLOR
    v_color = a_color;
#endif
    
#ifdef MODEL_VERTEX_NORMAL
    v_normal = a_normal;
#endif
    
#ifdef MODEL_VERTEX_TEXCOORD
    v_texcoord = a_texcoord;
#endif
    
#ifdef MODEL_VERTEX_TANGENT
    v_tangent = a_tangent;
    vec3 worldTangent = a_tangent.xyz;
    vec3 worldBiTangent = cross(v_normal, worldTangent);// * sign(a_tangent.w);
    v_tangentToWorld = mat3(normalize(worldTangent), normalize(worldBiTangent), normalize(v_normal));
#endif
    
#ifdef LIGHT_SHADOWMAP
    v_lightCoord = u_lightMatrix * v_position;
#endif
    
    gl_Position = u_modelViewProjectionMatrix * v_position;
}
)";

const std::string default_scene_frag0 = R"(

#ifdef GL_ES
precision mediump float;
#endif

uniform vec3    u_camera;
uniform vec2    u_resolution;

varying vec4    v_position;

#ifdef MODEL_VERTEX_COLOR
varying vec4    v_color;
#endif

#ifdef MODEL_VERTEX_NORMAL
varying vec3    v_normal;
#endif

#ifdef MODEL_VERTEX_TEXCOORD
varying vec2    v_texcoord;
#endif

#ifdef MODEL_VERTEX_TANGENT
varying mat3    v_tangentToWorld;
varying vec4    v_tangent;
#endif

// #define MATERIAL_ANISOTROPY 0.9
// #define MATERIAL_ANISOTROPY_DIRECTION vec3(0.2, 0.4, 0.23)

// #define DIFFUSE_BURLEY
// #define DIFFUSE_LAMBERT
// #define DIFFUSE_ORENNAYAR
// #define SPECULAR_GAUSSIAN
// #define SPECULAR_BLECKMANN
// #define SPECULAR_COOKTORRANCE
// #define SPECULAR_PHONG
// #define SPECULAR_BLINNPHONG
// #define LIGHT_SHADOWMAP_BIAS 0.005

#ifdef PLATFORM_RPI
#define TARGET_MOBILE
#endif

#ifndef TONEMAP_FNC
#define TONEMAP_FNC tonemap_reinhard
#endif


#ifndef FNC_MYPOW
#define FNC_MYPOW

float myPow(float a, float b) {
        return a / ((1. - b) * a + b);
}

#endif

#ifndef FNC_SATURATE
#define FNC_SATURATE

#define saturate(x)        clamp(x, 0.0, 1.0)

#endif

#if !defined(TARGET_MOBILE) && !defined(GAMMA)
#define GAMMA 2.2
#endif

#ifndef FNC_LINEAR2GAMMA
#define FNC_LINEAR2GAMMA
vec3 linear2gamma(in vec3 v) {
#ifdef GAMMA
  return pow(v, vec3(1. / GAMMA));
#else
  // assume gamma 2.0
  return sqrt(v);
#endif
}

vec4 linear2gamma(in vec4 v) {
  return vec4(linear2gamma(v.rgb), v.a);
}

float linear2gamma(in float v) {
#ifdef GAMMA
  return pow(v, 1. / GAMMA);
#else
  // assume gamma 2.0
  return sqrt(v);
#endif
}
#endif

#ifndef HEADER_LIGHT
#define HEADER_LIGHT

uniform vec3        u_light;
uniform vec3        u_lightColor;
uniform float       u_lightFalloff;
uniform float       u_lightIntensity;

#ifdef LIGHT_SHADOWMAP
uniform sampler2D   u_lightShadowMap;
uniform mat4        u_lightMatrix;
varying vec4        v_lightCoord;
#endif

#endif

#ifndef FNC_TEXTURESHADOW
#define FNC_TEXTURESHADOW

float textureShadow(const sampler2D _shadowMap, in vec4 _coord) {
    vec3 shadowCoord = _coord.xyz / _coord.w;
    return texture2D(_shadowMap, shadowCoord.xy).r;
}

float textureShadow(const sampler2D _shadowMap, in vec3 _coord) {
    return textureShadow(_shadowMap, vec4(_coord, 1.0));
}

float textureShadow(const sampler2D depths, vec2 uv, float compare){
    return step(compare, texture2D(depths, uv).r );
}

float textureShadow(const sampler2D _shadowMap) {
    #ifdef LIGHT_SHADOWMAP
    return textureShadow(_shadowMap, v_lightCoord);
    #else
    return 1.0;
    #endif
}

float textureShadow() {
    #ifdef LIGHT_SHADOWMAP
    return textureShadow(u_lightShadowMap);
    #else
    return 1.0;
    #endif
}

#endif


#ifndef FNC_SHADOW
#define FNC_SHADOW

#ifdef LIGHT_SHADOWMAP
//------------------------------------------------------------------------------
// Shadowing configuration
//------------------------------------------------------------------------------

#define SHADOW_SAMPLING_HARD              0
#define SHADOW_SAMPLING_PCF_LOW           1
#define SHADOW_SAMPLING_PCF_MEDIUM        2
#define SHADOW_SAMPLING_PCF_HIGH          3

#define SHADOW_SAMPLING_ERROR_DISABLED   0
#define SHADOW_SAMPLING_ERROR_ENABLED    1

#define SHADOW_RECEIVER_PLANE_DEPTH_BIAS_DISABLED   0
#define SHADOW_RECEIVER_PLANE_DEPTH_BIAS_ENABLED    1

#define SHADOW_RECEIVER_PLANE_DEPTH_BIAS_MIN_SAMPLING_METHOD    SHADOW_SAMPLING_PCF_MEDIUM

#ifdef TARGET_MOBILE
  #define SHADOW_SAMPLING_METHOD            SHADOW_SAMPLING_PCF_LOW
  #define SHADOW_SAMPLING_ERROR             SHADOW_SAMPLING_ERROR_DISABLED
  #define SHADOW_RECEIVER_PLANE_DEPTH_BIAS  SHADOW_RECEIVER_PLANE_DEPTH_BIAS_DISABLED
#else
  #define SHADOW_SAMPLING_METHOD            SHADOW_SAMPLING_PCF_HIGH
  #define SHADOW_SAMPLING_ERROR             SHADOW_SAMPLING_ERROR_DISABLED
  #define SHADOW_RECEIVER_PLANE_DEPTH_BIAS  SHADOW_RECEIVER_PLANE_DEPTH_BIAS_DISABLED
#endif

#if SHADOW_SAMPLING_ERROR == SHADOW_SAMPLING_ERROR_ENABLED
  #undef SHADOW_RECEIVER_PLANE_DEPTH_BIAS
  #define SHADOW_RECEIVER_PLANE_DEPTH_BIAS  SHADOW_RECEIVER_PLANE_DEPTH_BIAS_ENABLED
#elif SHADOW_SAMPLING_METHOD < SHADOW_RECEIVER_PLANE_DEPTH_BIAS_MIN_SAMPLING_METHOD
  #undef SHADOW_RECEIVER_PLANE_DEPTH_BIAS
  #define SHADOW_RECEIVER_PLANE_DEPTH_BIAS  SHADOW_RECEIVER_PLANE_DEPTH_BIAS_DISABLED
#endif

#undef SHADOW_WIDTH
#define SHADOW_WIDTH 0.01

//------------------------------------------------------------------------------
// Shadow sampling methods
//------------------------------------------------------------------------------

vec2 computeReceiverPlaneDepthBias(const vec3 position) {
    // see: GDC '06: Shadow Mapping: GPU-based Tips and Techniques
    vec2 bias;
#if SHADOW_RECEIVER_PLANE_DEPTH_BIAS == SHADOW_RECEIVER_PLANE_DEPTH_BIAS_ENABLED
    vec3 dx = dFdx(position);
    vec3 dy = dFdy(position);
    bias = vec2(dy.y * dx.z - dx.y * dy.z, dx.x * dy.z - dy.x * dx.z);
    bias *= 1.0 / ((dx.x * dy.y) - (dx.y * dy.x));
#else
    bias = vec2(0.0);
#endif
    return bias;
}

float samplingBias(float depth, const vec2 rpdb, const vec2 texelSize) {
#if SHADOW_SAMPLING_ERROR == SHADOW_SAMPLING_ERROR_ENABLED
    float samplingError = min(dot(texelSize, abs(rpdb)), 0.01);
    depth -= samplingError;
#endif
    return depth;
}

float sampleDepth(const sampler2D map, vec2 base, vec2 dudv, float depth, vec2 rpdb) {
#if SHADOW_RECEIVER_PLANE_DEPTH_BIAS == SHADOW_RECEIVER_PLANE_DEPTH_BIAS_ENABLED
    #if SHADOW_SAMPLING_METHOD >= SHADOW_RECEIVER_PLANE_DEPTH_BIAS_MIN_SAMPLING_METHOD
    depth += dot(dudv, rpdb);
    #endif
#endif

    float t = v_lightCoord.z - 0.005;
#ifdef TARGET_MOBILE
    return step(t, textureShadow(map, vec3(base + dudv, clamp(depth, 0.0, 1.0))));
#else
    return smoothstep(t-SHADOW_WIDTH, t+SHADOW_WIDTH, textureShadow(map, vec3(base + dudv, clamp(depth, 0.0, 1.0))));
#endif
}

#if SHADOW_SAMPLING_METHOD == SHADOW_SAMPLING_HARD
float ShadowSample_Hard(const sampler2D map, const vec2 size, const vec3 position) {
    vec2 rpdb = computeReceiverPlaneDepthBias(position);
    float depth = samplingBias(position.z, rpdb, vec2(1.0) / size);
    return step(v_lightCoord.z - 0.005, textureShadow(map, vec3(position.xy, depth)));
}
#endif

#if SHADOW_SAMPLING_METHOD == SHADOW_SAMPLING_PCF_LOW
float ShadowSample_PCF_Low(const sampler2D map, const vec2 size, vec3 position) {
    //  Castaño, 2013, "Shadow Mapping Summary Part 1"
    vec2 texelSize = vec2(1.0) / size;

    // clamp position to avoid overflows below, which cause some GPUs to abort
    position.xy = clamp(position.xy, vec2(-1.0), vec2(2.0));

    vec2 offset = vec2(0.5);
    vec2 uv = (position.xy * size) + offset;
    vec2 base = (floor(uv) - offset) * texelSize;
    vec2 st = fract(uv);

    vec2 uw = vec2(3.0 - 2.0 * st.x, 1.0 + 2.0 * st.x);
    vec2 vw = vec2(3.0 - 2.0 * st.y, 1.0 + 2.0 * st.y);

    vec2 u = vec2((2.0 - st.x) / uw.x - 1.0, st.x / uw.y + 1.0);
    vec2 v = vec2((2.0 - st.y) / vw.x - 1.0, st.y / vw.y + 1.0);

    u *= texelSize.x;
    v *= texelSize.y;

    vec2 rpdb = computeReceiverPlaneDepthBias(position);

    float depth = samplingBias(position.z, rpdb, texelSize);
    float sum = 0.0;

    sum += uw.x * vw.x * sampleDepth(map, base, vec2(u.x, v.x), depth, rpdb);
    sum += uw.y * vw.x * sampleDepth(map, base, vec2(u.y, v.x), depth, rpdb);

    sum += uw.x * vw.y * sampleDepth(map, base, vec2(u.x, v.y), depth, rpdb);
    sum += uw.y * vw.y * sampleDepth(map, base, vec2(u.y, v.y), depth, rpdb);

    return sum * (1.0 / 16.0);
}
#endif

#if SHADOW_SAMPLING_METHOD == SHADOW_SAMPLING_PCF_MEDIUM
float ShadowSample_PCF_Medium(const sampler2D map, const vec2 size, vec3 position) {
    //  Castaño, 2013, "Shadow Mapping Summary Part 1"
    vec2 texelSize = vec2(1.0) / size;

    // clamp position to avoid overflows below, which cause some GPUs to abort
    position.xy = clamp(position.xy, vec2(-1.0), vec2(2.0));

    vec2 offset = vec2(0.5);
    vec2 uv = (position.xy * size) + offset;
    vec2 base = (floor(uv) - offset) * texelSize;
    vec2 st = fract(uv);

    vec3 uw = vec3(4.0 - 3.0 * st.x, 7.0, 1.0 + 3.0 * st.x);
    vec3 vw = vec3(4.0 - 3.0 * st.y, 7.0, 1.0 + 3.0 * st.y);

    vec3 u = vec3((3.0 - 2.0 * st.x) / uw.x - 2.0, (3.0 + st.x) / uw.y, st.x / uw.z + 2.0);
    vec3 v = vec3((3.0 - 2.0 * st.y) / vw.x - 2.0, (3.0 + st.y) / vw.y, st.y / vw.z + 2.0);

    u *= texelSize.x;
    v *= texelSize.y;

    vec2 rpdb = computeReceiverPlaneDepthBias(position);

    float depth = samplingBias(position.z, rpdb, texelSize);
    float sum = 0.0;

    sum += uw.x * vw.x * sampleDepth(map, base, vec2(u.x, v.x), depth, rpdb);
    sum += uw.y * vw.x * sampleDepth(map, base, vec2(u.y, v.x), depth, rpdb);
    sum += uw.z * vw.x * sampleDepth(map, base, vec2(u.z, v.x), depth, rpdb);

    sum += uw.x * vw.y * sampleDepth(map, base, vec2(u.x, v.y), depth, rpdb);
    sum += uw.y * vw.y * sampleDepth(map, base, vec2(u.y, v.y), depth, rpdb);
    sum += uw.z * vw.y * sampleDepth(map, base, vec2(u.z, v.y), depth, rpdb);

    sum += uw.x * vw.z * sampleDepth(map, base, vec2(u.x, v.z), depth, rpdb);
    sum += uw.y * vw.z * sampleDepth(map, base, vec2(u.y, v.z), depth, rpdb);
    sum += uw.z * vw.z * sampleDepth(map, base, vec2(u.z, v.z), depth, rpdb);

    return sum * (1.0 / 144.0);
}
#endif

#if SHADOW_SAMPLING_METHOD == SHADOW_SAMPLING_PCF_HIGH
float ShadowSample_PCF_High(const sampler2D map, const vec2 size, vec3 position) {
    //  Castaño, 2013, "Shadow Mapping Summary Part 1"
    vec2 texelSize = vec2(1.0) / size;

    // clamp position to avoid overflows below, which cause some GPUs to abort
    position.xy = clamp(position.xy, vec2(-1.0), vec2(2.0));

    vec2 offset = vec2(0.5);
    vec2 uv = (position.xy * size) + offset;
    vec2 base = (floor(uv) - offset) * texelSize;
    vec2 st = fract(uv);

    vec4 uw = vec4(
         5.0 * st.x - 6.0,
         11.0 * st.x - 28.0,
        -(11.0 * st.x + 17.0),
        -(5.0 * st.x + 1.0));
    vec4 vw = vec4(
         5.0 * st.y - 6.0,
         11.0 * st.y - 28.0,
        -(11.0 * st.y + 17.0),
        -(5.0 * st.y + 1.0));

    vec4 u = vec4(
         (4.0 * st.x - 5.0) / uw.x - 3.0,
         (4.0 * st.x - 16.0) / uw.y - 1.0,
        -(7.0 * st.x + 5.0) / uw.z + 1.0,
        -st.x / uw.w + 3.0);
    vec4 v = vec4(
         (4.0 * st.y - 5.0) / vw.x - 3.0,
         (4.0 * st.y - 16.0) / vw.y - 1.0,
        -(7.0 * st.y + 5.0) / vw.z + 1.0,
        -st.y / vw.w + 3.0);

    u *= texelSize.x;
    v *= texelSize.y;

    vec2 rpdb = computeReceiverPlaneDepthBias(position);

    float depth = samplingBias(position.z, rpdb, texelSize);
    float sum = 0.0;

    sum += uw.x * vw.x * sampleDepth(map, base, vec2(u.x, v.x), depth, rpdb);
    sum += uw.y * vw.x * sampleDepth(map, base, vec2(u.y, v.x), depth, rpdb);
    sum += uw.z * vw.x * sampleDepth(map, base, vec2(u.z, v.x), depth, rpdb);
    sum += uw.w * vw.x * sampleDepth(map, base, vec2(u.w, v.x), depth, rpdb);

    sum += uw.x * vw.y * sampleDepth(map, base, vec2(u.x, v.y), depth, rpdb);
    sum += uw.y * vw.y * sampleDepth(map, base, vec2(u.y, v.y), depth, rpdb);
    sum += uw.z * vw.y * sampleDepth(map, base, vec2(u.z, v.y), depth, rpdb);
    sum += uw.w * vw.y * sampleDepth(map, base, vec2(u.w, v.y), depth, rpdb);

    sum += uw.x * vw.z * sampleDepth(map, base, vec2(u.x, v.z), depth, rpdb);
    sum += uw.y * vw.z * sampleDepth(map, base, vec2(u.y, v.z), depth, rpdb);
    sum += uw.z * vw.z * sampleDepth(map, base, vec2(u.z, v.z), depth, rpdb);
    sum += uw.w * vw.z * sampleDepth(map, base, vec2(u.w, v.z), depth, rpdb);

    sum += uw.x * vw.w * sampleDepth(map, base, vec2(u.x, v.w), depth, rpdb);
    sum += uw.y * vw.w * sampleDepth(map, base, vec2(u.y, v.w), depth, rpdb);
    sum += uw.z * vw.w * sampleDepth(map, base, vec2(u.z, v.w), depth, rpdb);
    sum += uw.w * vw.w * sampleDepth(map, base, vec2(u.w, v.w), depth, rpdb);

    return sum * (1.0 / 2704.0);
}
#endif

//------------------------------------------------------------------------------
// Shadow sampling dispatch
//------------------------------------------------------------------------------

/**
 * Samples the light visibility at the specified position in light (shadow)
 * space. The output is a filtered visibility factor that can be used to multiply
 * the light intensity.
 */

float shadow( const sampler2D map, const vec3 shadowPosition) {
    // vec2 size = vec2(textureSize(shadowMap, 0));
    vec2 size = vec2(LIGHT_SHADOWMAP_SIZE);
#if SHADOW_SAMPLING_METHOD == SHADOW_SAMPLING_HARD
    return ShadowSample_Hard(map, size, shadowPosition);
#elif SHADOW_SAMPLING_METHOD == SHADOW_SAMPLING_PCF_LOW
    return ShadowSample_PCF_Low(map, size, shadowPosition);
#elif SHADOW_SAMPLING_METHOD == SHADOW_SAMPLING_PCF_MEDIUM
    return ShadowSample_PCF_Medium(map, size, shadowPosition);
#elif SHADOW_SAMPLING_METHOD == SHADOW_SAMPLING_PCF_HIGH
    return ShadowSample_PCF_High(map, size, shadowPosition);
#endif
}

float shadow() {
    vec3 shadowPosition = v_lightCoord.xyz * (1.0 / v_lightCoord.w);
    return shadow(LIGHT_SHADOWMAP, shadowPosition);
}

#else

float shadow( const sampler2D map, const vec3 shadowPosition) {
    return 1.0;
}

float shadow() {
    return 1.0;
}    

#endif
#endif

#ifndef FNC_REFLECTION
#define FNC_REFLECTION

vec3 reflection(vec3 _V, vec3 _N, float _roughness) {
        // Reflect
#ifdef MATERIAL_ANISOTROPY
    vec3  anisotropicT = MATERIAL_ANISOTROPY_DIRECTION;
    vec3  anisotropicB = MATERIAL_ANISOTROPY_DIRECTION;

#ifdef MODERL_VERTEX_TANGENT
    anisotropicT = normalize(v_tangentToWorld * MATE RIAL_ANISOTROPY_DIRECTION);
    anisotropicB = normalize(cross(v_tangentToWorld[2], anisotropicT));
#endif

    vec3  anisotropyDirection = MATERIAL_ANISOTROPY >= 0.0 ? anisotropicB : anisotropicT;
    vec3  anisotropicTangent  = cross(anisotropyDirection, _V);
    vec3  anisotropicNormal   = cross(anisotropicTangent, anisotropyDirection);
    float bendFactor          = abs(MATERIAL_ANISOTROPY) * saturate(5.0 * _roughness);
    vec3  bentNormal          = normalize(mix(_N, anisotropicNormal, bendFactor));
    return reflect(-_V, bentNormal);
#else
    return reflect(-_V, _N);
#endif

}

#endif

#ifndef TARGET_MOBILE
#define IBL_SPECULAR_OCCLUSION
#endif

#ifndef FNC_SPECULARAO
#define FNC_SPECULARAO
float specularAO(float NoV, float ao, float roughness) {
#if !defined(TARGET_MOBILE)
    return saturate(pow(NoV + ao, exp2(-16.0 * roughness - 1.0)) - 1.0 + ao);
#else
    return 1.0;
#endif
}
#endif


#ifndef FNC_ENVBRDFAPPROX
#define FNC_ENVBRDFAPPROX

vec3 envBRDFApprox(vec3 _specularColor, float _NoV, float _roughness) {
    vec4 c0 = vec4( -1, -0.0275, -0.572, 0.022 );
    vec4 c1 = vec4( 1, 0.0425, 1.04, -0.04 );
    vec4 r = _roughness * c0 + c1;
    float a004 = min( r.x * r.x, exp2( -9.28 * _NoV ) ) * r.x + r.y;
    vec2 AB = vec2( -1.04, 1.04 ) * a004 + r.zw;
    return _specularColor * AB.x + AB.y;
}

#endif

#if !defined(TARGET_MOBILE) && !defined(GAMMA)
#define GAMMA 2.2
#endif

#ifndef FNC_GAMMA2LINEAR
#define FNC_GAMMA2LINEAR
float gamma2linear(in float v) {
#ifdef GAMMA
  return pow(v, GAMMA);
#else
  // assume gamma 2.0
  return v * v;
#endif
}

vec3 gamma2linear(in vec3 v) {
#ifdef GAMMA
  return pow(v, vec3(GAMMA));
#else
  // assume gamma 2.0
  return v * v;
#endif
}

vec4 gamma2linear(in vec4 v) {
  return vec4(gamma2linear(v.rgb), v.a);
}
#endif

#ifndef FNC_MATERIAL_BASECOLOR
#define FNC_MATERIAL_BASECOLOR

#ifdef MATERIAL_BASECOLORMAP
uniform sampler2D MATERIAL_BASECOLORMAP;
#endif

vec4 materialBaseColor() {
    vec4 base = vec4(1.0);
    
#if defined(MATERIAL_BASECOLORMAP) && defined(MODEL_VERTEX_TEXCOORD)
    vec2 uv = v_texcoord.xy;
    #if defined(MATERIAL_BASECOLORMAP_OFFSET)
    uv += (MATERIAL_BASECOLORMAP_OFFSET).xy;
    #endif
    #if defined(MATERIAL_BASECOLORMAP_SCALE)
    uv *= (MATERIAL_BASECOLORMAP_SCALE).xy;
    #endif
    base = gamma2linear( texture2D(MATERIAL_BASECOLORMAP, uv) );

#elif defined(MATERIAL_BASECOLOR)
    base = MATERIAL_BASECOLOR;

#endif

#if defined(MODEL_VERTEX_COLOR)
    base *= v_color;
#endif

    return base;
}

#endif)";

const std::string default_scene_frag1 = R"(

#ifndef FNC_MATERIAL_SPECULAR
#define FNC_MATERIAL_SPECULAR

#ifdef MATERIAL_SPECULARMAP
uniform sampler2D MATERIAL_SPECULARMAP;
#endif

vec3 materialSpecular() {
    vec3 spec = vec3(0.04);
#if defined(MATERIAL_SPECULARMAP) && defined(MODEL_VERTEX_TEXCOORD)
    vec2 uv = v_texcoord.xy;
    #if defined(MATERIAL_SPECULARMAP_OFFSET)
    uv += (MATERIAL_SPECULARMAP_OFFSET).xy;
    #endif
    #if defined(MATERIAL_SPECULARMAP_SCALE)
    uv *= (MATERIAL_SPECULARMAP_SCALE).xy;
    #endif
    spec = texture2D(MATERIAL_SPECULARMAP, uv).rgb;
#elif defined(MATERIAL_SPECULAR)
    spec = MATERIAL_SPECULAR;
#endif
    return spec;
}

#endif



#ifndef FNC_MATERIAL_EMISSIVE
#define FNC_MATERIAL_EMISSIVE

#ifdef MATERIAL_EMISSIVEMAP
uniform sampler2D MATERIAL_EMISSIVEMAP;
#endif

vec3 materialEmissive() {
    vec3 emission = vec3(0.0);

#if defined(MATERIAL_EMISSIVEMAP) && defined(MODEL_VERTEX_TEXCOORD)
    vec2 uv = v_texcoord.xy;
    #if defined(MATERIAL_EMISSIVEMAP_OFFSET)
    uv += (MATERIAL_EMISSIVEMAP_OFFSET).xy;
    #endif
    #if defined(MATERIAL_EMISSIVEMAP_SCALE)
    uv *= (MATERIAL_EMISSIVEMAP_SCALE).xy;
    #endif
    emission = gamma2linear(texture2D(MATERIAL_EMISSIVEMAP, uv)).rgb;

#elif defined(MATERIAL_EMISSIVE)
    emission = MATERIAL_EMISSIVE;
#endif

    return emission;
}

#endif



#ifndef FNC_MATERIAL_OCCLUSION
#define FNC_MATERIAL_OCCLUSION

#ifdef MATERIAL_OCCLUSIONMAP
uniform sampler2D MATERIAL_OCCLUSIONMAP;
#endif

#if defined(MATERIAL_OCCLUSIONROUGHNESSMETALLICMAP) && !defined(MATERIAL_OCCLUSIONROUGHNESSMETALLICMAP_UNIFORM)
#define MATERIAL_OCCLUSIONROUGHNESSMETALLICMAP_UNIFORM
uniform sampler2D MATERIAL_OCCLUSIONROUGHNESSMETALLICMAP;
#endif

float materialOcclusion() {
    float occlusion = 1.0;

#if defined(MATERIAL_OCCLUSIONMAP) && defined(MODEL_VERTEX_TEXCOORD)
    vec2 uv = v_texcoord.xy;
    occlusion = texture2D(MATERIAL_OCCLUSIONMAP, uv).r;
#elif defined(MATERIAL_OCCLUSIONROUGHNESSMETALLICMAP) && defined(MODEL_VERTEX_TEXCOORD)
    vec2 uv = v_texcoord.xy;
    occlusion = texture2D(MATERIAL_OCCLUSIONROUGHNESSMETALLICMAP, uv).r;
#endif

#if defined(MATERIAL_OCCLUSIONMAP_STRENGTH)
    occlusion *= MATERIAL_OCCLUSIONMAP_STRENGTH;
#endif

    return occlusion;
}

#endif



#ifndef FNC_MATERIAL_NORMAL
#define FNC_MATERIAL_NORMAL

#ifdef MATERIAL_NORMALMAP
uniform sampler2D MATERIAL_NORMALMAP;
#endif

#ifdef MATERIAL_BUMPMAP_NORMALMAP
uniform sampler2D MATERIAL_BUMPMAP_NORMALMAP;
#endif

vec3 materialNormal() {
    vec3 normal = vec3(0.0, 0.0, 1.0);

#ifdef MODEL_VERTEX_NORMAL
    normal = v_normal;

    #if defined(MODEL_VERTEX_TANGENT) && defined(MODEL_VERTEX_TEXCOORD) && defined(MATERIAL_NORMALMAP) 
    vec2 uv = v_texcoord.xy;
        #if defined(MATERIAL_NORMALMAP_OFFSET)
    uv += (MATERIAL_NORMALMAP_OFFSET).xy;
        #endif
        #if defined(MATERIAL_NORMALMAP_SCALE)
    uv *= (MATERIAL_NORMALMAP_SCALE).xy;
        #endif
    normal = texture2D(MATERIAL_NORMALMAP, uv).xyz;
    normal = v_tangentToWorld * (normal * 2.0 - 1.0);

    #elif defined(MODEL_VERTEX_TANGENT) && defined(MODEL_VERTEX_TEXCOORD) && defined(MATERIAL_BUMPMAP_NORMALMAP)
    vec2 uv = v_texcoord.xy;
        #if defined(MATERIAL_BUMPMAP_OFFSET)
    uv += (MATERIAL_BUMPMAP_OFFSET).xy;
        #endif
        #if defined(MATERIAL_BUMPMAP_SCALE)
    uv *= (MATERIAL_BUMPMAP_SCALE).xy;
        #endif
    normal = v_tangentToWorld * (texture2D(MATERIAL_BUMPMAP_NORMALMAP, uv).xyz * 2.0 - 1.0);
    #endif
    
#endif

    return normal;
}
#endif




#ifndef TOMETALLIC_MIN_REFLECTANCE
#define TOMETALLIC_MIN_REFLECTANCE 0.04
#endif

#ifndef FNC_TOMETALLIC
#define FNC_TOMETTALIC

// Gets metallic factor from specular glossiness workflow inputs 
float toMetallic(vec3 diffuse, vec3 specular, float maxSpecular) {
    float perceivedDiffuse = sqrt(0.299 * diffuse.r * diffuse.r + 0.587 * diffuse.g * diffuse.g + 0.114 * diffuse.b * diffuse.b);
    float perceivedSpecular = sqrt(0.299 * specular.r * specular.r + 0.587 * specular.g * specular.g + 0.114 * specular.b * specular.b);
    if (perceivedSpecular < TOMETALLIC_MIN_REFLECTANCE) {
        return 0.0;
    }
    float a = TOMETALLIC_MIN_REFLECTANCE;
    float b = perceivedDiffuse * (1.0 - maxSpecular) / (1.0 - TOMETALLIC_MIN_REFLECTANCE) + perceivedSpecular - 2.0 * TOMETALLIC_MIN_REFLECTANCE;
    float c = TOMETALLIC_MIN_REFLECTANCE - perceivedSpecular;
    float D = max(b * b - 4.0 * a * c, 0.0);
    return clamp((-b + sqrt(D)) / (2.0 * a), 0.0, 1.0);
}

float toMetallic(vec3 diffuse, vec3 specular) {
    float maxSpecula = max(max(specular.r, specular.g), specular.b);
    return toMetallic(diffuse, specular, maxSpecula);
}

#endif


#ifndef FNC_MATERIAL_METALLIC
#define FNC_MATERIAL_METALLIC

#ifdef MATERIAL_METALLICMAP
uniform sampler2D MATERIAL_METALLICMAP;
#endif

#if defined(MATERIAL_ROUGHNESSMETALLICMAP) && !defined(MATERIAL_ROUGHNESSMETALLICMAP_UNIFORM)
#define MATERIAL_ROUGHNESSMETALLICMAP_UNIFORM
uniform sampler2D MATERIAL_ROUGHNESSMETALLICMAP;
#endif

#if defined(MATERIAL_OCCLUSIONROUGHNESSMETALLICMAP) && !defined(MATERIAL_OCCLUSIONROUGHNESSMETALLICMAP_UNIFORM)
#define MATERIAL_OCCLUSIONROUGHNESSMETALLICMAP_UNIFORM
uniform sampler2D MATERIAL_OCCLUSIONROUGHNESSMETALLICMAP;
#endif
    
float materialMetallic() {
    float metallic = 0.0;

#if defined(MATERIAL_METALLICMAP) && defined(MODEL_VERTEX_TEXCOORD)
    vec2 uv = v_texcoord.xy;
    #if defined(MATERIAL_METALLICMAP_OFFSET)
    uv += (MATERIAL_METALLICMAP_OFFSET).xy;
    #endif
    #if defined(MATERIAL_METALLICMAP_SCALE)
    uv *= (MATERIAL_METALLICMAP_SCALE).xy;
    #endif
    metallic = texture2D(MATERIAL_METALLICMAP, uv).b;

#elif defined(MATERIAL_ROUGHNESSMETALLICMAP) && defined(MODEL_VERTEX_TEXCOORD)
    vec2 uv = v_texcoord.xy;
    metallic = texture2D(MATERIAL_ROUGHNESSMETALLICMAP, uv).b;

#elif defined(MATERIAL_OCCLUSIONROUGHNESSMETALLICMAP) && defined(MODEL_VERTEX_TEXCOORD)
    vec2 uv = v_texcoord.xy;
    metallic = texture2D(MATERIAL_OCCLUSIONROUGHNESSMETALLICMAP, uv).b;

#elif defined(MATERIAL_METALLIC)
    metallic = MATERIAL_METALLIC;

#else
    vec3 diffuse = materialBaseColor().rgb;
    vec3 specular = materialSpecular();
    metallic = toMetallic(diffuse, specular);
#endif

    return metallic;
}

#endif



#ifndef FNC_MATERIAL_ROUGHNESS
#define FNC_MATERIAL_ROUGHNESS

#ifdef MATERIAL_ROUGHNESSMAP
uniform sampler2D MATERIAL_ROUGHNESSMAP;
#endif

#if defined(MATERIAL_ROUGHNESSMETALLICMAP) && !defined(MATERIAL_ROUGHNESSMETALLICMAP_UNIFORM)
#define MATERIAL_ROUGHNESSMETALLICMAP_UNIFORM
uniform sampler2D MATERIAL_ROUGHNESSMETALLICMAP;
#endif

#if defined(MATERIAL_OCCLUSIONROUGHNESSMETALLICMAP) && !defined(MATERIAL_OCCLUSIONROUGHNESSMETALLICMAP_UNIFORM)
#define MATERIAL_OCCLUSIONROUGHNESSMETALLICMAP_UNIFORM
uniform sampler2D MATERIAL_OCCLUSIONROUGHNESSMETALLICMAP;
#endif

float materialRoughness() {
    float roughness = 0.1;

#if defined(MATERIAL_ROUGHNESSMAP) && defined(MODEL_VERTEX_TEXCOORD)
    vec2 uv = v_texcoord.xy;
    #if defined(MATERIAL_ROUGHNESSMAP_OFFSET)
    uv += (MATERIAL_ROUGHNESSMAP_OFFSET).xy;
    #endif
    #if defined(MATERIAL_ROUGHNESSMAP_SCALE)
    uv *= (MATERIAL_ROUGHNESSMAP_SCALE).xy;
    #endif
    roughness = texture2D(MATERIAL_ROUGHNESSMAP, uv).g;

#elif defined(MATERIAL_ROUGHNESSMETALLICMAP) && defined(MODEL_VERTEX_TEXCOORD)
    vec2 uv = v_texcoord.xy;
    roughness = texture2D(MATERIAL_ROUGHNESSMETALLICMAP, uv).g;

#elif defined(MATERIAL_OCCLUSIONROUGHNESSMETALLICMAP) && defined(MODEL_VERTEX_TEXCOORD)
    vec2 uv = v_texcoord.xy;
    roughness = texture2D(MATERIAL_OCCLUSIONROUGHNESSMETALLICMAP, uv).g;

#elif defined(MATERIAL_ROUGHNESS)
    roughness = MATERIAL_ROUGHNESS;

#endif

    return roughness;
}

#endif




#ifndef FNC_TOSHININESS
#define FNC_TOSHININESS

float toShininess(float roughness, float metallic) {
    float _smooth = .95 - roughness * 0.5;
    _smooth *= _smooth;
    _smooth *= _smooth;
    return _smooth * (80. + 160. * (1.0-metallic));
}

#endif


#ifndef FNC_MATERIAL_SHININESS
#define FNC_MATERIAL_SHININESS

float materialShininess() {
    float shininess = 15.0;

#ifdef MATERIAL_SHININESS
    shininess = MATERIAL_SHININESS;

#elif defined(MATERIAL_METALLIC) && defined(MATERIAL_ROUGHNESS)
    float roughness = materialRoughness();
    float metallic = materialMetallic();

    shininess = toShininess(roughness, metallic);
#endif
    return shininess;
}

#endif


#ifndef STR_MATERIAL
#define STR_MATERIAL
struct Material {
    vec4    baseColor;
    vec3    emissive;
    vec3    normal;
    
    vec3    f0;
    float   reflectance;

    float   roughness;
    float   metallic;

    float   ambientOcclusion;

#if defined(MATERIAL_CLEARCOAT_THICKNESS)
    float   clearCoat;
    float   clearCoatRoughness;
    #if defined(MATERIAL_CLEARCOAT_THICKNESS_NORMAL)
    vec3    clearCoatNormal;
    #endif
#endif

#if defined(SHADING_MODEL_SUBSURFACE)
    float   thickness;
    float   subsurfacePower;
#endif

#if defined(SHADING_MODEL_CLOTH)
    vec3    sheenColor;
#endif

#if defined(MATERIAL_SUBSURFACE_COLOR)
    vec3    subsurfaceColor;
#endif

#if defined(SHADING_MODEL_SPECULAR_GLOSSINESS)
    vec3    specularColor;
    float   glossiness;
#endif
};
#endif

#ifndef FNC_MATERIAL_INIT
#define FNC_MATERIAL_INIT

void initMaterial(out Material _mat) {
    _mat.baseColor = materialBaseColor();
    _mat.emissive = materialEmissive();
    _mat.normal = materialNormal();
    _mat.f0 = vec3(0.04);

    _mat.roughness = materialRoughness();
    _mat.metallic = materialMetallic();

    _mat.reflectance = 0.5;
    _mat.ambientOcclusion = materialOcclusion();

#if defined(MATERIAL_CLEARCOAT_THICKNESS)
    _mat.clearCoat = MATERIAL_CLEARCOAT_THICKNESS;
    _mat.clearCoatRoughness = MATERIAL_CLEARCOAT_ROUGHNESS;
#if defined(MATERIAL_CLEARCOAT_THICKNESS_NORMAL)
    _mat.clearCoatNormal = vec3(0.0, 0.0, 1.0);
#endif
#endif

#if defined(SHADING_MODEL_SUBSURFACE)
    _mat.thickness = 0.5;
    _mat.subsurfacePower = 12.234;
    _mat.subsurfaceColor = vec3(1.0);
#endif

#if defined(SHADING_MODEL_CLOTH)
    _mat.sheenColor = sqrt(_mat.baseColor.rgb);
#endif

#if defined(MATERIAL_SUBSURFACE_COLOR)
    _mat.subsurfaceColor = vec3(0.0);
#endif

}

Material MaterialInit() {
    Material mat;
    initMaterial(mat);
    return mat;
}

#endif





#ifndef FNC_POW5
#define FNC_POW5
float pow5(in float x) {
    float x2 = x * x;
    return x2 * x2 * x;
}

vec2 pow5(in vec2 x) {
    vec2 x2 = x * x;
    return x2 * x2 * x;
}

vec3 pow5(in vec3 x) {
    vec3 x2 = x * x;
    return x2 * x2 * x;
}

vec4 pow5(in vec4 x) {
    vec4 x2 = x * x;
    return x2 * x2 * x;
}
#endif



#ifndef FNC_SCHLICK
#define FNC_SCHLICK

// Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"
vec3 schlick(const vec3 f0, float f90, float VoH) {
    float f = pow5(1.0 - VoH);
    return f + f0 * (f90 - f);
}

vec3 schlick(vec3 f0, vec3 f90, float VoH) {
    return f0 + (f90 - f0) * pow5(1.0 - VoH);
}

float schlick(float f0, float f90, float VoH) {
    return f0 + (f90 - f0) * pow5(1.0 - VoH);
}

#endif



#ifndef HEADER_IBL
#define HEADER_IBL
uniform samplerCube u_cubeMap;
uniform vec3        u_SH[9];
uniform float       u_iblLuminance;
#endif


// #define TONEMAP_FNC tonemap_linear
// #define TONEMAP_FNC tonemap_reinhard
// #define TONEMAP_FNC tonemap_unreal
// #define TONEMAP_FNC tonemap_ACES
// #define TONEMAP_FNC tonemap_ACES_rec2020_1k
// #define TONEMAP_FNC tonemap_displayRange


/*
function: luminance
description: Computes the luminance of the specified linear RGB color using the luminance coefficients from Rec. 709.
use: luminance(<vec3|vec4> color)
*/

#ifndef FNC_LUMINANCE
#define FNC_LUMINANCE
float luminance(const vec3 linear) {
    return dot(linear, vec3(0.2126, 0.7152, 0.0722));
}

float luminance(const vec4 linear) {
    return dot(linear.rgb, vec3(0.2126, 0.7152, 0.0722));
}
#endif


#ifndef TONEMAP_FNC
    #ifdef TARGET_MOBILE
        #ifdef HAS_HARDWARE_CONVERSION_FUNCTION
            #define TONEMAP_FNC tonemap_ACES
        #else
            #define TONEMAP_FNC tonemap_unreal
        #endif
    #else
        #define TONEMAP_FNC     tonemap_ACES
    #endif
#endif

#ifndef FNC_TONEMAP
#define FNC_TONEMAP

//------------------------------------------------------------------------------
// Tone-mapping operators for LDR output
//------------------------------------------------------------------------------

vec3 tonemap_linear(const vec3 x) {
    return x;
}

vec3 tonemap_reinhard(const vec3 x) {
    // Reinhard et al. 2002, "Photographic Tone Reproduction for Digital Images", Eq. 3
    return x / (1.0 + luminance(x));
}

vec3 tonemap_unreal(const vec3 x) {
    // Unreal, Documentation: "Color Grading"
    // Adapted to be close to Tonemap_ACES, with similar range
    // Gamma 2.2 correction is baked in, don't use with sRGB conversion!
    return x / (x + 0.155) * 1.019;
}

vec3 tonemap_ACES(const vec3 x) {
    // Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

vec3 uncharted2Tonemap(const vec3 x) {
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 tonemap_uncharted(const vec3 color) {
    const float W = 11.2;
    const float exposureBias = 2.0;
    vec3 curr = uncharted2Tonemap(exposureBias * color);
    vec3 whiteScale = 1.0 / uncharted2Tonemap(vec3(W));
    return curr * whiteScale;
}

//------------------------------------------------------------------------------
// Tone-mapping operators for HDR output
//------------------------------------------------------------------------------

vec3 tonemap_ACES_rec2020_1k(const vec3 x) {
    // Narkowicz 2016, "HDR Display – First Steps"
    const float a = 15.8;
    const float b = 2.12;
    const float c = 1.2;
    const float d = 5.92;
    const float e = 1.9;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}
)";

const std::string default_scene_frag2 = R"(
//------------------------------------------------------------------------------
// Debug tone-mapping operators, for LDR output
//------------------------------------------------------------------------------

/**
 * Converts the input HDR RGB color into one of 16 debug colors that represent
 * the pixel's exposure. When the output is cyan, the input color represents
 * middle gray (18% exposure). Every exposure stop above or below middle gray
 * causes a color shift.
 *
 * The relationship between exposures and colors is:
 *
 * -5EV  - black
 * -4EV  - darkest blue
 * -3EV  - darker blue
 * -2EV  - dark blue
 * -1EV  - blue
 *  OEV  - cyan
 * +1EV  - dark green
 * +2EV  - green
 * +3EV  - yellow
 * +4EV  - yellow-orange
 * +5EV  - orange
 * +6EV  - bright red
 * +7EV  - red
 * +8EV  - magenta
 * +9EV  - purple
 * +10EV - white
 */

#ifndef PLATFORM_RPI
vec3 tonemap_displayRange(const vec3 x) {

    // 16 debug colors + 1 duplicated at the end for easy indexing
    vec3 debugColors[17];
    debugColors[0] = vec3(0.0, 0.0, 0.0);         // black
    debugColors[1] = vec3(0.0, 0.0, 0.1647);      // darkest blue
    debugColors[2] = vec3(0.0, 0.0, 0.3647);      // darker blue
    debugColors[3] = vec3(0.0, 0.0, 0.6647);      // dark blue
    debugColors[4] = vec3(0.0, 0.0, 0.9647);      // blue
    debugColors[5] = vec3(0.0, 0.9255, 0.9255);   // cyan
    debugColors[6] = vec3(0.0, 0.5647, 0.0);      // dark green
    debugColors[7] = vec3(0.0, 0.7843, 0.0);      // green
    debugColors[8] = vec3(1.0, 1.0, 0.0);         // yellow
    debugColors[9] = vec3(0.90588, 0.75294, 0.0); // yellow-orange
    debugColors[10] = vec3(1.0, 0.5647, 0.0);      // orange
    debugColors[11] = vec3(1.0, 0.0, 0.0);         // bright red
    debugColors[12] = vec3(0.8392, 0.0, 0.0);      // red
    debugColors[13] = vec3(1.0, 0.0, 1.0);         // magenta
    debugColors[14] = vec3(0.6, 0.3333, 0.7882);   // purple
    debugColors[15] = vec3(1.0, 1.0, 1.0);         // white
    debugColors[16] = vec3(1.0, 1.0, 1.0);         // white

    // The 5th color in the array (cyan) represents middle gray (18%)
    // Every stop above or below middle gray causes a color shift
    float v = log2(luminance(x) / 0.18);
    v = clamp(v + 5.0, 0.0, 15.0);
    int index = int(v);
    return mix(debugColors[index], debugColors[index + 1], v - float(index));
}
#endif

//------------------------------------------------------------------------------
// Tone-mapping dispatch
//------------------------------------------------------------------------------

/**
 * Tone-maps the specified RGB color. The input color must be in linear HDR and
 * pre-exposed. Our HDR to LDR tone mapping operators are designed to tone-map
 * the range [0..~8] to [0..1].
 */
vec3 tonemap(const vec3 x) {
    return TONEMAP_FNC(x);
}

vec3 tonemap(const vec4 x) {
    return TONEMAP_FNC(x.xyz);
}

#endif




#ifndef FNC_FAKECUBE
#define FNC_FAKECUBE

vec3 fakeCube(vec3 _normal, float _shininnes) {
    vec3 rAbs = abs(_normal);
    return vec3( myPow(max(max(rAbs.x, rAbs.y), rAbs.z) + 0.005, _shininnes) );
}

vec3 fakeCube(vec3 _normal) {
    return fakeCube(_normal, materialShininess() );
}

#endif


#ifndef ENVMAP_MAX_MIP_LEVEL
#define ENVMAP_MAX_MIP_LEVEL 8.0
#endif

#ifndef FNC_ENVMAP
#define FNC_ENVMAP

vec3 envMap(vec3 _normal, float _roughness, float _metallic) {

#if defined(SCENE_CUBEMAP)
    float lod = ENVMAP_MAX_MIP_LEVEL * _roughness;
    return textureCube( SCENE_CUBEMAP, _normal, lod).rgb;

#else
    return fakeCube(_normal, toShininess(_roughness, _metallic));

#endif
}

vec3 envMap(vec3 _normal, float _roughness) {
    return envMap(_normal, _roughness, 1.0);
}

#endif



#ifndef SPHERICALHARMONICS_BANDS
// Number of spherical harmonics bands (1, 2 or 3)
#if defined(TARGET_MOBILE)
#define SPHERICALHARMONICS_BANDS           2
#else
#define SPHERICALHARMONICS_BANDS           3
#endif
#endif

// #ifndef SCENE_SH_ARRAY
// #define SCENE_SH_ARRAY u_SH
// #endif

#ifndef FNC_SPHERICALHARMONICS
#define FNC_SPHERICALHARMONICS

vec3 sphericalHarmonics(const vec3 n) {
#ifdef SCENE_SH_ARRAY
    return max(
           0.282095 * SCENE_SH_ARRAY[0]
#if SPHERICALHARMONICS_BANDS >= 2
        + -0.488603 * SCENE_SH_ARRAY[1] * (n.y)
        +  0.488603 * SCENE_SH_ARRAY[2] * (n.z)
        + -0.488603 * SCENE_SH_ARRAY[3] * (n.x)
#endif
#if SPHERICALHARMONICS_BANDS >= 3
        +  1.092548 * SCENE_SH_ARRAY[4] * (n.y * n.x)
        + -1.092548 * SCENE_SH_ARRAY[5] * (n.y * n.z)
        +  0.315392 * SCENE_SH_ARRAY[6] * (3.0 * n.z * n.z - 1.0)
        + -1.092548 * SCENE_SH_ARRAY[7] * (n.z * n.x)
        +  0.546274 * SCENE_SH_ARRAY[8] * (n.x * n.x - n.y * n.y)
#endif
        , 0.0);
#else
    return vec3(1.0);
#endif
}

#endif


#ifndef FNC_FRESNEL
#define FNC_FRESNEL

vec3 fresnel(const vec3 f0, float LoH) {
#if defined(TARGET_MOBILE)
    return schlick(f0, 1.0, LoH);
#else
    float f90 = saturate(dot(f0, vec3(50.0 * 0.33)));
    return schlick(f0, f90, LoH);
#endif
}

vec3 fresnel(vec3 _R, vec3 _f0, float _NoV) {
    vec3 frsnl = fresnel(_f0, _NoV);
    // frsnl = schlick(_f0, vec3(1.0), _NoV);

    vec3 reflectColor = vec3(0.0);
    #if defined(SCENE_SH_ARRAY)
    reflectColor = tonemap( sphericalHarmonics(_R) );
    #else
    reflectColor = fakeCube(_R);
    #endif

    return reflectColor * frsnl;
}

#endif



// #define SPECULAR_BLINNPHONG
// #define SPECULAR_PHONG
// #define SPECULAR_GGX
// #define SPECULAR_BLECKMANN
// #define SPECULAR_GAUSSIAN
// #define SPECULAR_COOKTORRANCE
// #define DIFFUSE_BURLEY
// #define DIFFUSE_ORENNAYAR
// #define DIFFUSE_LAMBERT




#ifndef SPECULAR_POW
#ifdef TARGET_MOBILE
#define SPECULAR_POW(A,B) myPow(A,B)
#else
#define SPECULAR_POW(A,B) pow(A,B)
#endif
#endif

#ifndef FNC_SPECULAR_PHONG
#define FNC_SPECULAR_PHONG 

// https://github.com/glslify/glsl-specular-phong
float specularPhong(vec3 L, vec3 N, vec3 V, float shininess) {
    vec3 R = reflect(L, N); // 2.0 * dot(N, L) * N - L;
    return SPECULAR_POW(max(0.0, dot(R, -V)), shininess);
}

#endif



#ifndef SPECULAR_POW
#ifdef TARGET_MOBILE
#define SPECULAR_POW(A,B) myPow(A,B)
#else
#define SPECULAR_POW(A,B) pow(A,B)
#endif
#endif

#ifndef FNC_SPECULAR_BLINNPHONG
#define FNC_SPECULAR_BLINNPHONG

// https://github.com/glslify/glsl-specular-blinn-phong
float specularBlinnPhong(vec3 L, vec3 N, vec3 V, float shininess) {
    // halfVector
    vec3 H = normalize(L + V);
    return SPECULAR_POW(max(0.0, dot(N, H)), shininess);
}

#endif



#ifndef FNC_BECKMANN
#define FNC_BECKMANN

// https://github.com/glslify/glsl-specular-beckmann
float beckmann(float _NoH, float roughness) {
    float NoH = max(_NoH, 0.0001);
    float cos2Alpha = NoH * NoH;
    float tan2Alpha = (cos2Alpha - 1.0) / cos2Alpha;
    float roughness2 = roughness * roughness;
    float denom = 3.141592653589793 * roughness2 * cos2Alpha * cos2Alpha;
    return exp(tan2Alpha / roughness2) / denom;
}

#endif


#ifndef SPECULAR_POW
#ifdef TARGET_MOBILE
#define SPECULAR_POW(A,B) myPow(A,B)
#else
#define SPECULAR_POW(A,B) pow(A,B)
#endif
#endif

#ifndef FNC_SPECULAR_COOKTORRANCE
#define FNC_SPECULAR_COOKTORRANCE

// https://github.com/stackgl/glsl-specular-cook-torrance
float specularCookTorrance(vec3 _L, vec3 _N, vec3 _V, float _NoV, float _NoL, float _roughness, float _fresnel) {
    float NoV = max(_NoV, 0.0);
    float NoL = max(_NoL, 0.0);

    //Half angle vector
    vec3 H = normalize(_L + _V);

    //Geometric term
    float NoH = max(dot(_N, H), 0.0);
    float VoH = max(dot(_V, H), 0.000001);
    float LoH = max(dot(_L, H), 0.000001);

    float x = 2.0 * NoH / VoH;
    float G = min(1.0, min(x * NoV, x * NoL));
    
    //Distribution term
    float D = beckmann(NoH, _roughness);

    //Fresnel term
    float F = SPECULAR_POW(1.0 - NoV, _fresnel);

    //Multiply terms and done
    return  G * F * D / max(3.14159265 * NoV * NoL, 0.000001);
}

// https://github.com/glslify/glsl-specular-cook-torrance
float specularCookTorrance(vec3 L, vec3 N, vec3 V, float roughness, float fresnel) {
    float NoV = max(dot(N, V), 0.0);
    float NoL = max(dot(N, L), 0.0);
    return specularCookTorrance(L, N, V, NoV, NoL, roughness, fresnel);
}

#endif


#ifndef FNC_SPECULAR_GAUSSIAN
#define FNC_SPECULAR_GAUSSIAN

// https://github.com/glslify/glsl-specular-gaussian
float specularGaussian(vec3 L, vec3 N, vec3 V, float roughness) {
  vec3 H = normalize(L + V);
  float theta = acos(dot(H, N));
  float w = theta / roughness;
  return exp(-w*w);
}

#endif



#ifndef FNC_SPECULAR_BECKMANN
#define FNC_SPECULAR_BECKMANN

float specularBeckmann(vec3 L, vec3 N, vec3 V, float roughness) {
    float NoH = dot(N, normalize(L + V));
    return beckmann(NoH, roughness);
}

#endif



#ifndef ONE_OVER_PI
#define ONE_OVER_PI 0.31830988618
#endif


#ifndef FNC_SATURATEMEDIUMP
#define FNC_SATURATEMEDIUMP

#ifndef MEDIUMP_FLT_MAX
#define MEDIUMP_FLT_MAX    65504.0
#endif

#ifdef TARGET_MOBILE
#define saturateMediump(x) min(x, MEDIUMP_FLT_MAX)
#else
#define saturateMediump(x) x
#endif

#endif


#ifndef FNC_GGX
#define FNC_GGX

    // Walter et al. 2007, "Microfacet Models for Refraction through Rough Surfaces"

    // In mediump, there are two problems computing 1.0 - NoH^2
    // 1) 1.0 - NoH^2 suffers floating point cancellation when NoH^2 is close to 1 (highlights)
    // 2) NoH doesn't have enough precision around 1.0
    // Both problem can be fixed by computing 1-NoH^2 in highp and providing NoH in highp as well

    // However, we can do better using Lagrange's identity:
    //      ||a x b||^2 = ||a||^2 ||b||^2 - (a . b)^2
    // since N and H are unit vectors: ||N x H||^2 = 1.0 - NoH^2
    // This computes 1.0 - NoH^2 directly (which is close to zero in the highlights and has
    // enough precision).
    // Overall this yields better performance, keeping all computations in mediump

float GGX(float NoH, float linearRoughness) {
    float oneMinusNoHSquared = 1.0 - NoH * NoH;
    float a = NoH * linearRoughness;
    float k = linearRoughness / (oneMinusNoHSquared + a * a);
    float d = k * k * ONE_OVER_PI;
    return saturateMediump(d);
}

float GGX(vec3 N, vec3 H, float NoH, float linearRoughness) {
    vec3 NxH = cross(N, H);
    float oneMinusNoHSquared = dot(NxH, NxH);

    float a = NoH * linearRoughness;
    float k = linearRoughness / (oneMinusNoHSquared + a * a);
    float d = k * k * ONE_OVER_PI;
    return saturateMediump(d);
}

#endif




#ifndef FNC_SMITH_GGX_CORRELATED
#define FNC_SMITH_GGX_CORRELATED

float smithGGXCorrelated(float NoV, float NoL, float roughness) {
    // Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
    float a2 = roughness * roughness;
    // TODO: lambdaV can be pre-computed for all the lights, it should be moved out of this function
    float lambdaV = NoL * sqrt((NoV - a2 * NoV) * NoV + a2);
    float lambdaL = NoV * sqrt((NoL - a2 * NoL) * NoL + a2);
    float v = 0.5 / (lambdaV + lambdaL);
    // a2=0 => v = 1 / 4*NoL*NoV   => min=1/4, max=+inf
    // a2=1 => v = 1 / 2*(NoL+NoV) => min=1/4, max=+inf
    // clamp to the maximum value representable in mediump
    return saturateMediump(v);
}

float smithGGXCorrelated_Fast(float NoV, float NoL, float roughness) {
    // Hammon 2017, "PBR Diffuse Lighting for GGX+Smith Microsurfaces"
    float v = 0.5 / mix(2.0 * NoL * NoV, NoL + NoV, roughness);
    return saturateMediump(v);
}

#endif


#ifndef FNC_SPECULAR_GGX
#define FNC_SPECULAR_GGX

float specularGGX(vec3 _L, vec3 _N, vec3 _V, float _NoV, float _NoL, float _roughness, float _fresnel) {
    float NoV = max(_NoV, 0.0);
    float NoL = max(_NoL, 0.0);
    vec3 s = normalize(u_light - v_position.xyz);

    vec3 H = normalize(s + _V);
    float LoH = saturate(dot(s, H));
    float NoH = saturate(dot(_N, H));

    // float NoV, float NoL, float roughness
    float linearRoughness =  _roughness * _roughness;
    float D = GGX(NoH, linearRoughness);
    float V = smithGGXCorrelated(_NoV, NoL,linearRoughness);
    float F = fresnel(vec3(_fresnel), LoH).r;

    return (D * V) * F;
}

float specularGGX(vec3 L, vec3 N, vec3 V, float roughness, float fresnel) {
    float NoV = max(dot(N, V), 0.0);
    float NoL = max(dot(N, L), 0.0);
    return specularGGX(L, N, V, NoV, NoL, roughness, fresnel);
}

#endif



#ifndef FNC_SPECULAR
#define FNC_SPECULAR

float specular(vec3 L, vec3 N, vec3 V, float roughness) {

#if defined(SPECULAR_GAUSSIAN)
    return specularGaussian(L, N, V, roughness);
    
#elif defined(SPECULAR_BLECKMANN)
    return specularBeckmann(L, N, V, roughness);

#elif defined(SPECULAR_COOKTORRANCE)
    float f0 = 0.04;
    return specularCookTorrance(L, N, V, roughness, f0);

#elif defined(SPECULAR_PHONG)
    float shininess = toShininess(roughness, 0.0);
    return specularPhong(L, N, V, shininess);

#elif defined(SPECULAR_BLINNPHONG)
    float shininess = toShininess(roughness, 0.0);
    return specularBlinnPhong(L, N, V, shininess);
#else

    #ifdef TARGET_MOBILE
    float shininess = toShininess(roughness, 0.0);
    return specularBlinnPhong(L, N, V, shininess);
    #else
    float f0 = 0.04;
    return specularCookTorrance(L, N, V, roughness, f0);
    #endif  

#endif
}

float specular(vec3 L, vec3 N, vec3 V, float NoV, float NoL, float roughness, float fresnel) {
#if defined(SPECULAR_GAUSSIAN)
    return specularGaussian(L, N, V, roughness);

#elif defined(SPECULAR_BLECKMANN)
    return specularBeckmann(L, N, V, roughness);

#elif defined(SPECULAR_COOKTORRANCE)
    return specularCookTorrance(L, N, V, roughness, fresnel);

#elif defined(SPECULAR_GGX)
    return specularGGX(L, N, V, roughness, fresnel);

#elif defined(SPECULAR_PHONG)
    float shininess = toShininess(roughness, 0.0);
    return specularPhong(L, N, V, shininess);

#elif defined(SPECULAR_BLINNPHONG)
    float shininess = toShininess(roughness, 0.0);
    return specularBlinnPhong(L, N, V, shininess);
#else

    #ifdef TARGET_MOBILE
    float shininess = toShininess(roughness, 0.0);
    return specularBlinnPhong(L, N, V, shininess);
    #else
    return specularCookTorrance(L, N, V, roughness, fresnel);
    #endif  

#endif
}

#endif



#ifndef FNC_DIFFUSE_LAMBERT
#define FNC_DIFFUSE_LAMBERT

// https://github.com/glslify/glsl-diffuse-lambert
float diffuseLambert(vec3 L, vec3 N) {
    return max(0.0, dot(N, L));
}

#endif


#ifndef FNC_DIFFUSE_ORENNAYAR
#define FNC_DIFFUSE_ORENNAYAR

float diffuseOrenNayar(vec3 L, vec3 N, vec3 V, float NoV, float NoL, float roughness) {
    float LoV = dot(L, V);
    
    float s = LoV - NoL * NoV;
    float t = mix(1.0, max(NoL, NoV), step(0.0, s));

    float sigma2 = roughness * roughness;
    float A = 1.0 + sigma2 * (1.0 / (sigma2 + 0.13) + 0.5 / (sigma2 + 0.33));
    float B = 0.45 * sigma2 / (sigma2 + 0.09);

    return max(0.0, NoL) * (A + B * s / t);
}

// https://github.com/glslify/glsl-diffuse-oren-nayar
float diffuseOrenNayar(vec3 L, vec3 N, vec3 V, float roughness) {
    float NoV = (dot(N, V), 0.001);
    float NoL = (dot(N, L), 0.001);
    return diffuseOrenNayar(L, N, V, NoV, NoL, roughness);
}

#endif
)";

const std::string default_scene_frag3 = R"(

#ifndef FNC_DIFFUSE_BURLEY
#define FNC_DIFFUSE_BURLEY

float diffuseBurley(float NoV, float NoL, float LoH, float linearRoughness) {
    // Burley 2012, "Physically-Based Shading at Disney"
    float f90 = 0.5 + 2.0 * linearRoughness * LoH * LoH;
    float lightScatter = schlick(1.0, f90, NoL);
    float viewScatter  = schlick(1.0, f90, NoV);
    return lightScatter * viewScatter;
}

float diffuseBurley(vec3 L, vec3 N, vec3 V, float NoV, float NoL, float roughness) {
    float LoH = max(dot(L, normalize(L + V)), 0.001);
    return diffuseBurley(NoV, NoL, LoH, roughness * roughness);
}

float diffuseBurley(vec3 L, vec3 N, vec3 V, float roughness) {
    vec3 H = normalize(V + L);
    float NoV = clamp(dot(N, V), 0.001, 1.0);
    float NoL = clamp(dot(N, L), 0.001, 1.0);
    float LoH = clamp(dot(L, H), 0.001, 1.0);

    return diffuseBurley(NoV, NoL, LoH, roughness * roughness);
}

#endif


#ifndef FNC_DIFFUSE
#define FNC_DIFFUSE

float diffuse(vec3 L, vec3 N, vec3 V, float roughness) {
#if defined(DIFFUSE_ORENNAYAR)
    return diffuseOrenNayar(L, N, V, roughness);

#elif defined(DIFFUSE_BURLEY)
    return diffuseBurley(L, N, V, roughness);

#else
    return diffuseLambert(L, N);

#endif    
}

float diffuse(vec3 _L, vec3 _N, vec3 _V, float _NoV, float _NoL, float _roughness) {
#if defined(DIFFUSE_ORENNAYAR)
    return diffuseOrenNayar(_L, _N, _V, _NoV, _NoL, _roughness);

#elif defined(DIFFUSE_BURLEY)
    return diffuseBurley(_L, _N, _V, _NoV, _NoL, _roughness);

#else
    return diffuseLambert(_L, _N);
#endif    
}

#endif


#ifndef FNC_FALLOFF
#define FNC_FALLOFF

float falloff(float _dist, float _lightRadius) {
    float att = clamp(1.0 - _dist * _dist / (_lightRadius * _lightRadius), 0.0, 1.0);
    att *= att;
    return att;
}

#endif


#ifndef FNC_LIGHT_POINT
#define FNC_LIGHT_POINT

void lightPoint(vec3 _diffuseColor, vec3 _specularColor, vec3 _N, vec3 _V, float _NoV, float _roughness, float _f0, out vec3 _diffuse, out vec3 _specular) {
    vec3 s = normalize(u_light - v_position.xyz);
    float NoL = dot(_N, s);

    float dif = diffuse(s, _N, _V, _NoV, NoL, _roughness) * ONE_OVER_PI;
    float spec = specular(s, _N, _V, _NoV, NoL, _roughness, _f0);

    float fall = 1.0;
    if (u_lightFalloff > 0.0)
        fall = falloff(length(u_light - v_position.xyz), u_lightFalloff);
    
    _diffuse = u_lightIntensity * (_diffuseColor * u_lightColor * dif * fall);
    _specular = u_lightIntensity * (_specularColor * u_lightColor * spec * fall);
}

#endif


#ifndef FNC_PBR
#define FNC_PBR

void lightWithShadow(vec3 _diffuseColor, vec3 _specularColor, vec3 _N, vec3 _V, float _NoV, float _roughness, float _f0, inout vec3 _diffuse, inout vec3 _specular) {
    vec3 lightDiffuse = vec3(0.0);
    vec3 lightSpecular = vec3(0.0);

    lightPoint(_diffuseColor, _specularColor, _N, _V, _NoV, _roughness, _f0, lightDiffuse, lightSpecular);
    // calcPointLight(_comp, lightDiffuse, lightSpecular);
    // calcDirectionalLight(_comp, lightDiffuse, lightSpecular);

    float shadows = 1.0;

#if defined(LIGHT_SHADOWMAP) && defined(LIGHT_SHADOWMAP_SIZE) && !defined(PLATFORM_RPI)
    shadows = shadow();
#endif

    _diffuse += lightDiffuse * shadows;
    _specular += saturate(lightSpecular) * shadows;
}

vec4 pbr(const Material _mat) {
    // Calculate Color

    vec3    diffuseColor = _mat.baseColor.rgb * (vec3(1.0) - _mat.f0) * (1.0 - _mat.metallic);
    vec3    specularColor = mix(_mat.f0, _mat.baseColor.rgb, _mat.metallic);

    vec3    N = _mat.normal;                             // Normal
    vec3    V = normalize(u_camera - v_position.xyz);   // View
    float NoV = dot(N, V);                            // Normal . View
    float f0  = max(_mat.f0.r, max(_mat.f0.g, _mat.f0.b));
    float roughness = _mat.roughness;
    
    // Reflect
    vec3    R = reflection(V, N, roughness);

    // Ambient Occlusion
    // ------------------------
    float ssao = 1.0;
#ifdef SCENE_SSAO
    ssao = texture2D(SCENE_SSAO, gl_FragCoord.xy/u_resolution).r;
#endif 
    float diffuseAO = min(_mat.ambientOcclusion, ssao);
    float specularAO = specularAO(NoV, diffuseAO, roughness);

    // Global Ilumination ( mage Based Lighting )
    // ------------------------
    vec3 E = envBRDFApprox(specularColor, NoV, roughness);

    vec3 Fr = vec3(0.0);
    Fr = tonemap( envMap(R, roughness, _mat.metallic) ) * E;
    Fr += fresnel(R, _mat.f0, NoV) * _mat.metallic;
    Fr *= specularAO;

    vec3 Fd = vec3(0.0);
    vec3 diffuseIrradiance = vec3(1.0);
    #if defined(SCENE_SH_ARRAY)
    diffuseIrradiance *= tonemap( sphericalHarmonics(N) );
    #endif
    Fd = diffuseColor * diffuseIrradiance * diffuseAO;// * (1.0 - E);

    // Local Ilumination
    // ------------------------
    vec3 lightDiffuse = vec3(0.0);
    vec3 lightSpecular = vec3(0.0);
    lightWithShadow(diffuseColor, specularColor, N, V, NoV, roughness, f0, lightDiffuse, lightSpecular);
    
    // Final Sum
    // ------------------------
    vec4 color = vec4(0.0);
    color.rgb += Fd * u_iblLuminance + lightDiffuse;     // Diffuse
    color.rgb += Fr * u_iblLuminance + lightSpecular;    // Specular
    color.rgb *= _mat.ambientOcclusion;
    color.rgb += _mat.emissive;
    color.a = _mat.baseColor.a;

    return linear2gamma(color);
}


#endif

void main(void) {
    Material mat = MaterialInit();

#ifdef FLOOR 
    vec2 st = v_texcoord - 0.5;
    st *= 7.0;
    vec4 t = vec4(  fract(st),
                    floor(st));
    vec2 c = mod(t.zw, 2.0);
    float p = abs(c.x-c.y) * 0.5;
             
    mat.baseColor += p;
    mat.roughness = 0.5 + p * 0.5;
#endif
    
    gl_FragColor = pbr(mat);
}

)";

 extern std::string default_scene_frag;
