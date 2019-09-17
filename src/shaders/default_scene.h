#pragma once

#include "default_header.h"
#include "materials_functions.h"
#include "shadows_functions.h"

// DEFAULT SHADERS
// -----------------------------------------------------
const std::string default_scene_vert = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
uniform mat4 u_modelViewProjectionMatrix;\n\
\n\
attribute vec4  a_position;\n\
varying vec4    v_position;\n\
\n\
#ifdef MODEL_VERTEX_COLOR\n\
attribute vec4  a_color;\n\
varying vec4    v_color;\n\
#endif\n\
\n\
#ifdef MODEL_VERTEX_NORMAL\n\
attribute vec3  a_normal;\n\
varying vec3    v_normal;\n\
#endif\n\
\n\
#ifdef MODEL_VERTEX_TEXCOORD\n\
attribute vec2  a_texcoord;\n\
varying vec2    v_texcoord;\n\
#endif\n\
\n\
#ifdef MODEL_VERTEX_TANGENT\n\
attribute vec4  a_tangent;\n\
varying vec4    v_tangent;\n\
varying mat3    v_tangentToWorld;\n\
#endif\n\
\n\
#ifdef LIGHT_SHADOWMAP\n\
uniform mat4    u_lightMatrix;\n\
varying vec4    v_lightcoord;\n\
#endif\n\
\n\
void main(void) {\n\
    \n\
    v_position = a_position;\n\
    \n\
#ifdef MODEL_VERTEX_COLOR\n\
    v_color = a_color;\n\
#endif\n\
    \n\
#ifdef MODEL_VERTEX_NORMAL\n\
    v_normal = a_normal;\n\
#endif\n\
    \n\
#ifdef MODEL_VERTEX_TEXCOORD\n\
    v_texcoord = a_texcoord;\n\
#endif\n\
    \n\
#ifdef MODEL_VERTEX_TANGENT\n\
    v_tangent = a_tangent;\n\
    vec3 worldTangent = a_tangent.xyz;\n\
    vec3 worldBiTangent = cross(v_normal, worldTangent) * sign(a_tangent.w);\n\
    v_tangentToWorld = mat3(normalize(worldTangent), normalize(worldBiTangent), normalize(v_normal));\n\
#endif\n\
    \n\
#ifdef LIGHT_SHADOWMAP\n\
    v_lightcoord = u_lightMatrix * v_position;\n\
#endif\n\
    \n\
    gl_Position = u_modelViewProjectionMatrix * v_position;\n\
}\n\
";

const std::string default_scene_frag = default_header + materials_functions + shadows_functions + 
"struct Components {\n\
    vec3    diffuseColor;\n\
    vec3    specularColor;\n\
    vec3    N;  // Normal\n\
    vec3    V;  // View\n\
    vec3    R;  // Reflect\n\
    float   NoV;// Normal . View\n\
    float   roughness;\n\
};\n\
\n\
uniform samplerCube u_cubeMap;\n\
uniform vec3        u_SH[9];\n\
const float         numMips = 6.0;\n\
\n\
vec3 tonemap(const vec3 x) { return x / (x + 0.155) * 1.019; }\n\
vec3 sphericalHarmonics(vec3 _normal) {\n\
    return max( 0.282095 * u_SH[0]\n\
        + -0.488603 * u_SH[1] * (_normal.y)\n\
        +  0.488603 * u_SH[2] * (_normal.z)\n\
        + -0.488603 * u_SH[3] * (_normal.x)\n\
        , 0.0);\n\
}\n\
\n\
float myPow(float a, float b) { return a / ((1. - b) * a + b); }\n\
vec3 fakeCubeMap(vec3 _normal) {\n\
    vec3 rAbs = abs(_normal);\n\
    return vec3(1.0) * myPow(max(max(rAbs.x, rAbs.y), rAbs.z) + 0.005, getMaterialShininess());\n\
}\n\
\n\
// http://the-witness.net/news/2012/02/seamless-cube-map-filtering/\n\
vec3 fix_cube_lookup( vec3 v, float cube_size, float lod ) {\n\
    float M = max(max(abs(v.x), abs(v.y)), abs(v.z));\n\
    float scale = 1.0 - exp2(lod) / cube_size;\n\
    if (abs(v.x) != M) v.x *= scale;\n\
    if (abs(v.y) != M) v.y *= scale;\n\
    if (abs(v.z) != M) v.z *= scale;\n\
    return v;\n\
}\n\
\n\
vec3 prefilterEnvMap(vec3 _normal, float _roughness) {\n\
#if defined(SCENE_CUBEMAP)\n\
    vec4 color = mix(   textureCube(SCENE_CUBEMAP, _normal, (_roughness * numMips) ),\n\
                        textureCube(SCENE_CUBEMAP, _normal, min((_roughness * numMips) + 1.0, numMips)),\n\
                        fract(_roughness * numMips) );\n\
    return tonemap(color.rgb);\n\
#elif defined(DEBUG)\n\
    float cube_size = numMips * 10.;\n\
    return mix( fix_cube_lookup( _normal, cube_size, (_roughness * numMips) ),\n\
                fix_cube_lookup( _normal, cube_size, min((_roughness * numMips) + 1.0, numMips)),\n\
                fract(_roughness * numMips) );\n\
#else\n\
    return fakeCubeMap(_normal);\n\
#endif\n\
}\n\
\n\
vec3 envBRDFApprox(Components _comp) {\n\
    vec4 c0 = vec4( -1, -0.0275, -0.572, 0.022 );\n\
    vec4 c1 = vec4( 1, 0.0425, 1.04, -0.04 );\n\
    vec4 r = _comp.roughness * c0 + c1;\n\
    float a004 = min( r.x * r.x, exp2( -9.28 * _comp.NoV ) ) * r.x + r.y;\n\
    vec2 AB = vec2( -1.04, 1.04 ) * a004 + r.zw;\n\
    return _comp.specularColor * AB.x + AB.y;\n\
}\n\
\n\
vec3 approximateSpecularIBL(Components _comp) {\n\
    vec3 PrefilteredColor = prefilterEnvMap(_comp.R, _comp.roughness);\n\
    return _comp.specularColor * PrefilteredColor * envBRDFApprox(_comp);\n\
}\n\
\n\
// Fresnel\n\
vec3 fresnel(Components _comp, vec3 _f0) {\n\
    float base = 1.0 - clamp(_comp.NoV, 0.0, 0.99);\n\
    float exponential = pow(base, 5.0);\n\
    vec3 frsnl = _f0 + (vec3(1.0) - _f0) * exponential;\n\
    vec3 reflectColor = vec3(0.0);\n\
\n\
    #if defined(SCENE_SH_ARRAY)\n\
    reflectColor = tonemap(sphericalHarmonics(_comp.R));\n\
    #else\n\
    reflectColor = fakeCubeMap(_comp.R);\n\
    #endif\n\
\n\
    return reflectColor * frsnl;\n\
}\n\
\n\
// https://github.com/stackgl/glsl-specular-cook-torrance/blob/master/index.glsl\n\
float beckmannDistribution(float _x, float _roughness) {\n\
    float NdotH = max(_x, 0.0001);\n\
    float cos2Alpha = NdotH * NdotH;\n\
    float tan2Alpha = (cos2Alpha - 1.0) / cos2Alpha;\n\
    float roughness2 = max(_roughness * _roughness, 0.0001);\n\
    float denom = 3.141592653589793 * roughness2 * cos2Alpha * cos2Alpha;\n\
    return exp(tan2Alpha / roughness2) / denom;\n\
}\n\
\n\
// https://github.com/stackgl/glsl-specular-cook-torrance\n\
float cookTorranceSpecular(Components _comp, vec3 _lightDirection) {\n\
    float NoV = max(_comp.NoV, 0.0);\n\
    float NoL = max(dot(_comp.N, _lightDirection), 0.0);\n\
    vec3 H = normalize(_lightDirection + _comp.V);\n\
\n\
    //Geometric term\n\
    float NoH = max(dot(_comp.N, H), 0.0);\n\
    float VoH = max(dot(_comp.V, H), 0.000001);\n\
    float LoH = max(dot(_lightDirection, H), 0.000001);\n\
\n\
    float G1 = (2.0 * NoH * NoV) / VoH;\n\
    float G2 = (2.0 * NoH * NoL) / LoH;\n\
    float G = min(1.0, min(G1, G2));\n\
\n\
    //Distribution term\n\
    float D = beckmannDistribution(NoH, _comp.roughness);\n\
\n\
    //Fresnel term\n\
    float F = pow(1.0 - NoV, 0.02);\n\
\n\
    //Multiply terms and done\n\
    return  G * F * D / max(3.14159265 * NoV, 0.000001);\n\
}\n\
\n\
// https://github.com/stackgl/glsl-diffenable-oren-nayar\n\
float orenNayarDiffuse(Components _comp, vec3 _lightDirection) {\n\
    float LoV = dot(_lightDirection, _comp.V);\n\
    float NoL = dot(_comp.N, _lightDirection);\n\
\n\
    float s = LoV - NoL * _comp.NoV;\n\
    float t = mix(1.0, max(NoL, _comp.NoV), step(0.0, s));\n\
\n\
    float sigma2 = _comp.roughness * _comp.roughness;\n\
    float A = 1.0 + sigma2 * (1.0 / (sigma2 + 0.13) + 0.5 / (sigma2 + 0.33));\n\
    float B = 0.45 * sigma2 / (sigma2 + 0.09);\n\
\n\
    return max(0.0, NoL) * (A + B * s / t) / 3.14159265358979;\n\
}\n\
\n\
float falloff(float _dist, float _lightRadius) {\n\
    float att = clamp(1.0 - _dist * _dist / (_lightRadius * _lightRadius), 0.0, 1.0);\n\
    att *= att;\n\
    return att;\n\
}\n\
\n\
void calcPointLight(Components _comp, out vec3 _diffuse, out vec3 _specular) {\n\
    vec3 s = normalize(u_light - v_position.xyz);\n\
    float dif = orenNayarDiffuse(_comp, s);\n\
    float falloff = 1.0;\n\
    // falloff = Falloff(length(u_light - v_position.xyz), light.pointLightRadius);\n\
    float light_intensity = 2.0;\n\
    float spec = cookTorranceSpecular(_comp, s);\n\
    _diffuse = light_intensity * (_comp.diffuseColor * u_lightColor * dif * falloff);\n\
    _specular = light_intensity * (_comp.specularColor * u_lightColor * spec * falloff);\n\
}\n\
\n\
void lightWithShadow(Components _comp, out vec3 _diffuse, out vec3 _specular) {\n\
    vec3 lightDiffuse = vec3(0.0);\n\
    vec3 lightSpecular = vec3(0.0);\n\
\n\
    calcPointLight(_comp, lightDiffuse, lightSpecular);\n\
\n\
    float shadow = 1.0;\n\
\n\
#if defined(LIGHT_SHADOWMAP) && defined(LIGHT_SHADOWMAP_SIZE) && !defined(PLATFORM_RPI)\n\
    float bias = 0.005;\n\
    shadow *= textureShadowPCF(u_ligthShadowMap, vec2(LIGHT_SHADOWMAP_SIZE), v_lightcoord.xy, v_lightcoord.z - bias);\n\
    shadow = clamp(shadow, 0.0, 1.0);\n\
#endif\n\
\n\
    _diffuse = vec3(lightDiffuse * shadow);\n\
    _specular = clamp(lightSpecular, 0.0, 1.0) * shadow;\n\
}\n\
\n\
void main(void) {\n\
    vec3 albedo = getMaterialAlbedo();\n\
    vec3 emissionColor = getMaterialEmission();\n\
    vec3 f0 = getMaterialSpecular();\n\
    float roughness = getMaterialRoughness();\n\
    float metallic = getMaterialMetallic();\n\
\n\
    // Calculate Color\n\
    vec3 color = vec3(0.0);\n\
\n\
    Components comp;\n\
    comp.diffuseColor = albedo * (vec3(1.0) - f0) * (1.0 - metallic);\n\
    comp.specularColor = mix(f0, albedo, metallic);\n\
    comp.N = getMaterialNormal();\n\
    comp.V = normalize(u_camera - v_position.xyz);\n\
    comp.R = reflect(-comp.V, comp.N);\n\
    comp.NoV = dot(comp.N, comp.V);\n\
    comp.roughness = roughness;\n\
\n\
    vec3 diffuse = comp.diffuseColor * 0.3183098862; // Lambert\n\
    // IBL\n\
    #if defined(SCENE_SH_ARRAY)\n\
    diffuse *= tonemap( sphericalHarmonics(comp.N) );\n\
    #endif\n\
\n\
    vec3 specular = approximateSpecularIBL(comp) * metallic;\n\
    vec3 frsnl = vec3(0.0);\n\
    frsnl = fresnel(comp, f0) * metallic;\n\
\n\
    // Add light\n\
    vec3 lightDeffuse = vec3(0.0);\n\
    vec3 lightSpecular = vec3(0.0);\n\
    lightWithShadow(comp, lightDeffuse, lightSpecular);\n\
    diffuse += lightDeffuse;\n\
    specular += lightSpecular;\n\
\n\
    color = max(diffuse + specular + frsnl, emissionColor);\n\
    color = LINEARtoSRGB(color);\n\
    gl_FragColor = vec4(color, 1.0);\n\
}";
