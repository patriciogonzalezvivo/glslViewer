#pragma once

#include "default_header.h"
#include "materials_functions.h"
#include "shadows_functions.h"

// DEFAULT SHADERS
// -----------------------------------------------------
const std::string default_scene_vert = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform mat4 u_modelViewProjectionMatrix;

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
    vec3 worldBiTangent = cross(v_normal, worldTangent) * sign(a_tangent.w);
    v_tangentToWorld = mat3(normalize(worldTangent), normalize(worldBiTangent), normalize(v_normal));
#endif
    
#ifdef LIGHT_SHADOWMAP
    v_lightCoord = u_lightMatrix * v_position;
#endif
    
    gl_Position = u_modelViewProjectionMatrix * v_position;
}
)";

const std::string default_scene_frag = default_header + materials_functions + shadows_functions + 
R"(struct Components {
    vec3    diffuseColor;
    vec3    specularColor;
    vec3    N;  // Normal
    vec3    V;  // View
    vec3    R;  // Reflect
    float   NoV;// Normal . View
    float   roughness;
};

uniform samplerCube u_cubeMap;
uniform vec3        u_SH[9];
const float         numMips = 6.0;

vec3 tonemap(const vec3 x) { return x / (x + 0.155) * 1.019; }
vec3 sphericalHarmonics(vec3 _normal) {
    return max( 0.282095 * u_SH[0]
        + -0.488603 * u_SH[1] * (_normal.y)
        +  0.488603 * u_SH[2] * (_normal.z)
        + -0.488603 * u_SH[3] * (_normal.x)
        , 0.0);
}

float myPow(float a, float b) { return a / ((1. - b) * a + b); }
vec3 fakeCubeMap(vec3 _normal) {
    vec3 rAbs = abs(_normal);
    return vec3(1.0) * myPow(max(max(rAbs.x, rAbs.y), rAbs.z) + 0.005, getMaterialShininess());
}

// http://the-witness.net/news/2012/02/seamless-cube-map-filtering/
vec3 fix_cube_lookup( vec3 v, float cube_size, float lod ) {
    float M = max(max(abs(v.x), abs(v.y)), abs(v.z));
    float scale = 1.0 - exp2(lod) / cube_size;
    if (abs(v.x) != M) v.x *= scale;
    if (abs(v.y) != M) v.y *= scale;
    if (abs(v.z) != M) v.z *= scale;
    return v;
}

vec3 prefilterEnvMap(vec3 _normal, float _roughness) {
#if defined(SCENE_CUBEMAP)
    vec4 color = mix(   textureCube(SCENE_CUBEMAP, _normal, (_roughness * numMips) ),
                        textureCube(SCENE_CUBEMAP, _normal, min((_roughness * numMips) + 1.0, numMips)),
                        fract(_roughness * numMips) );
    return tonemap(color.rgb);
#elif defined(DEBUG)
    float cube_size = numMips * 10.;
    return mix( fix_cube_lookup( _normal, cube_size, (_roughness * numMips) ),
                fix_cube_lookup( _normal, cube_size, min((_roughness * numMips) + 1.0, numMips)),
                fract(_roughness * numMips) );
#else
    return fakeCubeMap(_normal);
#endif
}

vec3 envBRDFApprox(Components _comp) {
    vec4 c0 = vec4( -1, -0.0275, -0.572, 0.022 );
    vec4 c1 = vec4( 1, 0.0425, 1.04, -0.04 );
    vec4 r = _comp.roughness * c0 + c1;
    float a004 = min( r.x * r.x, exp2( -9.28 * _comp.NoV ) ) * r.x + r.y;
    vec2 AB = vec2( -1.04, 1.04 ) * a004 + r.zw;
    return _comp.specularColor * AB.x + AB.y;
}

vec3 approximateSpecularIBL(Components _comp) {
    vec3 PrefilteredColor = prefilterEnvMap(_comp.R, _comp.roughness);
    return _comp.specularColor * PrefilteredColor * envBRDFApprox(_comp);
}

// Fresnel
vec3 fresnel(Components _comp, vec3 _f0) {
    float base = 1.0 - clamp(_comp.NoV, 0.0, 0.99);
    float exponential = pow(base, 5.0);
    vec3 frsnl = _f0 + (vec3(1.0) - _f0) * exponential;
    vec3 reflectColor = vec3(0.0);

    #if defined(SCENE_SH_ARRAY)
    reflectColor = tonemap(sphericalHarmonics(_comp.R));
    #else
    reflectColor = fakeCubeMap(_comp.R);
    #endif

    return reflectColor * frsnl;
}

// https://github.com/stackgl/glsl-specular-cook-torrance/blob/master/index.glsl
float beckmannDistribution(float _x, float _roughness) {
    float NdotH = max(_x, 0.0001);
    float cos2Alpha = NdotH * NdotH;
    float tan2Alpha = (cos2Alpha - 1.0) / cos2Alpha;
    float roughness2 = max(_roughness * _roughness, 0.0001);
    float denom = 3.141592653589793 * roughness2 * cos2Alpha * cos2Alpha;
    return exp(tan2Alpha / roughness2) / denom;
}

// https://github.com/stackgl/glsl-specular-cook-torrance
float cookTorranceSpecular(Components _comp, vec3 _lightDirection) {
    float NoV = max(_comp.NoV, 0.0);
    float NoL = max(dot(_comp.N, _lightDirection), 0.0);
    vec3 H = normalize(_lightDirection + _comp.V);

    //Geometric term
    float NoH = max(dot(_comp.N, H), 0.0);
    float VoH = max(dot(_comp.V, H), 0.000001);
    float LoH = max(dot(_lightDirection, H), 0.000001);

    float G1 = (2.0 * NoH * NoV) / VoH;
    float G2 = (2.0 * NoH * NoL) / LoH;
    float G = min(1.0, min(G1, G2));

    //Distribution term
    float D = beckmannDistribution(NoH, _comp.roughness);

    //Fresnel term
    float F = pow(1.0 - NoV, 0.02);

    //Multiply terms and done
    return  G * F * D / max(3.14159265 * NoV, 0.000001);
}

// https://github.com/stackgl/glsl-diffenable-oren-nayar
float orenNayarDiffuse(Components _comp, vec3 _lightDirection) {
    float LoV = dot(_lightDirection, _comp.V);
    float NoL = dot(_comp.N, _lightDirection);

    float s = LoV - NoL * _comp.NoV;
    float t = mix(1.0, max(NoL, _comp.NoV), step(0.0, s));

    float sigma2 = _comp.roughness * _comp.roughness;
    float A = 1.0 + sigma2 * (1.0 / (sigma2 + 0.13) + 0.5 / (sigma2 + 0.33));
    float B = 0.45 * sigma2 / (sigma2 + 0.09);

    return max(0.0, NoL) * (A + B * s / t) / 3.14159265358979;
}

float falloff(float _dist, float _lightRadius) {
    float att = clamp(1.0 - _dist * _dist / (_lightRadius * _lightRadius), 0.0, 1.0);
    att *= att;
    return att;
}

void calcPointLight(Components _comp, out vec3 _diffuse, out vec3 _specular) {
    vec3 s = normalize(u_light - v_position.xyz);
    float dif = orenNayarDiffuse(_comp, s);
    float falloff = 1.0;
    // falloff = Falloff(length(u_light - v_position.xyz), light.pointLightRadius);
    float light_intensity = 2.0;
    float spec = cookTorranceSpecular(_comp, s);
    _diffuse = light_intensity * (_comp.diffuseColor * u_lightColor * dif * falloff);
    _specular = light_intensity * (_comp.specularColor * u_lightColor * spec * falloff);
}

void lightWithShadow(Components _comp, out vec3 _diffuse, out vec3 _specular) {
    vec3 lightDiffuse = vec3(0.0);
    vec3 lightSpecular = vec3(0.0);

    calcPointLight(_comp, lightDiffuse, lightSpecular);

    float shadow = 1.0;

#if defined(LIGHT_SHADOWMAP) && defined(LIGHT_SHADOWMAP_SIZE) && !defined(PLATFORM_RPI)
    float bias = 0.005;
    shadow *= textureShadowPCF(u_ligthShadowMap, vec2(LIGHT_SHADOWMAP_SIZE), v_lightCoord.xy, v_lightCoord.z - bias);
    shadow = clamp(shadow, 0.0, 1.0);
#endif

    _diffuse = vec3(lightDiffuse * shadow);
    _specular = clamp(lightSpecular, 0.0, 1.0) * shadow;
}

void main(void) {
    vec3 albedo = getMaterialAlbedo();
    vec3 emissionColor = getMaterialEmission();
    vec3 f0 = getMaterialSpecular();
    float roughness = getMaterialRoughness();
    float metallic = getMaterialMetallic();

    // Calculate Color
    vec3 color = vec3(0.0);

    Components comp;
    comp.diffuseColor = albedo * (vec3(1.0) - f0) * (1.0 - metallic);
    comp.specularColor = mix(f0, albedo, metallic);
    comp.N = getMaterialNormal();
    comp.V = normalize(u_camera - v_position.xyz);
    comp.R = reflect(-comp.V, comp.N);
    comp.NoV = dot(comp.N, comp.V);
    comp.roughness = roughness;

    vec3 diffuse = comp.diffuseColor * 0.3183098862; // Lambert
    // IBL
    #if defined(SCENE_SH_ARRAY)
    diffuse *= tonemap( sphericalHarmonics(comp.N) );
    #endif

    vec3 specular = approximateSpecularIBL(comp) * metallic;
    vec3 frsnl = vec3(0.0);
    frsnl = fresnel(comp, f0) * metallic;

    // Add light
    vec3 lightDeffuse = vec3(0.0);
    vec3 lightSpecular = vec3(0.0);
    lightWithShadow(comp, lightDeffuse, lightSpecular);
    diffuse += lightDeffuse;
    specular += lightSpecular;

    color = max(diffuse + specular + frsnl, emissionColor);
    color = LINEARtoSRGB(color);
    gl_FragColor = vec4(color, 1.0);
})";
