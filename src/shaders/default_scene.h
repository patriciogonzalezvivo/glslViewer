#pragma once

#include "textureShadow.h"
#include "sphericalHarmonics.h"


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
#ifdef MODEL_HAS_COLORS\n\
attribute vec4  a_color;\n\
varying vec4    v_color;\n\
#endif\n\
\n\
#ifdef MODEL_HAS_NORMALS\n\
attribute vec3  a_normal;\n\
varying vec3    v_normal;\n\
#endif\n\
\n\
#ifdef MODEL_HAS_TEXCOORDS\n\
attribute vec2  a_texcoord;\n\
varying vec2    v_texcoord;\n\
#endif\n\
\n\
#ifdef MODEL_HAS_TANGENTS\n\
attribute vec4  a_tangent;\n\
varying vec4    v_tangent;\n\
varying mat3    v_tangentToWorld;\n\
#endif\n\
\n\
#ifdef SHADOW_MAP\n\
uniform mat4    u_lightMatrix;\n\
varying vec4    v_lightcoord;\n\
#endif\n\
\n\
void main(void) {\n\
    \n\
    v_position = a_position;\n\
    \n\
#ifdef MODEL_HAS_COLORS\n\
    v_color = a_color;\n\
#endif\n\
    \n\
#ifdef MODEL_HAS_NORMALS\n\
    v_normal = a_normal;\n\
#endif\n\
    \n\
#ifdef MODEL_HAS_TEXCOORDS\n\
    v_texcoord = a_texcoord;\n\
#endif\n\
    \n\
#ifdef MODEL_HAS_TANGENTS\n\
    v_tangent = a_tangent;\n\
    vec3 worldTangent = a_tangent.xyz;\n\
    vec3 worldBiTangent = cross(v_normal, worldTangent) * sign(a_tangent.w);\n\
    v_tangentToWorld = mat3(normalize(worldTangent), normalize(worldBiTangent), normalize(v_normal));\n\
#endif\n\
    \n\
#ifdef SHADOW_MAP\n\
    v_lightcoord = u_lightMatrix * v_position;\n\
#endif\n\
    \n\
    gl_Position = u_modelViewProjectionMatrix * v_position;\n\
}\n\
";

std::string default_scene_frag = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
#ifdef MATERIAL_DIFFUSEMAP\n\
uniform sampler2D MATERIAL_DIFFUSEMAP;\n\
#endif\n\
\n\
#ifdef MATERIAL_BUMPMAP_NORMALMAP\n\
uniform sampler2D MATERIAL_BUMPMAP_NORMALMAP;\n\
#endif\n\
\n\
uniform vec3    u_light;\n\
uniform vec3    u_camera;\n\
\n\
varying vec4    v_position;\n\
\n\
#ifdef MODEL_HAS_COLORS\n\
varying vec4    v_color;\n\
#endif\n\
\n\
#ifdef MODEL_HAS_NORMALS\n\
varying vec3    v_normal;\n\
#endif\n\
\n\
#ifdef MODEL_HAS_TEXCOORDS\n\
varying vec2    v_texcoord;\n\
#endif\n\
\n\
#ifdef MODEL_HAS_TANGENTS\n\
varying mat3    v_tangentToWorld;\n\
varying vec4    v_tangent;\n\
#endif\n\
\n\
\n" + textureShadow + "\n\
\n\
#define saturate(x)        clamp(x, 0.0, 1.0)\n\
float   myPow(float a, float b) { return a / ((1. - b) * a + b); }\n\
vec3    myPow(vec3 a, float b) { return a / ((1. - b) * a + b); }\n\
\n\
\n" + sphericalHarmonics + "\n\
\n\
void main(void) {\n\
    vec3 color = vec3(0.5);\n\
\n\
    vec3 ambient = vec3(0.0, 0.0, .3);\n\
    vec3 diffuse = vec3(0.5);\n\
\n\
#ifdef MATERIAL_AMBIENT\n\
    ambient = MATERIAL_AMBIENT;\n\
#endif\n\
\n\
#ifdef MATERIAL_DIFFUSE\n\
    diffuse = MATERIAL_DIFFUSE;\n\
#endif\n\
\n\
#if defined(MODEL_HAS_COLORS)\n\
    diffuse *= v_color.rgb;\n\
#endif\n\
\n\
#if defined(MATERIAL_DIFFUSEMAP) && defined(MODEL_HAS_TEXCOORDS)\n\
    diffuse *= texture2D(MATERIAL_DIFFUSEMAP, v_texcoord.xy).rgb;\n\
#endif\n\
\n\
    vec3 specular = vec3(1.0);\n\
#ifdef MATERIAL_SPECULAR\n\
    specular = MATERIAL_SPECULAR;\n\
#endif\n\
    float shininess = 20.0;\n\
#ifdef MATERIAL_SHININESS\n\
    shininess = MATERIAL_SHININESS;\n\
#endif\n\
\n\
    vec3 emission = vec3(0.0);\n\
#ifdef MATERIAL_EMISSION\n\
    emission = MATERIAL_EMISSION;\n\
#endif\n\
\n\
#ifdef MODEL_HAS_NORMALS\n\
    vec3 l = normalize(u_light);\n\
    vec3 e = normalize(u_camera);\n\
    vec3 n = v_normal;\n\
    #if defined(MODEL_HAS_TANGENTS) && defined(MATERIAL_BUMPMAP_NORMALMAP)\n\
    n = (texture2D(MATERIAL_BUMPMAP_NORMALMAP, v_texcoord.xy).xyz * 2.0 - 1.0);\n\
    n = v_tangentToWorld * n;\n\
    #endif\n\
    n = normalize(n);\n\
    \n\
    float t = dot(n, l) * 0.5 + 0.5;\n\
\n\
#ifdef SH_ARRAY\n\
    vec3 sh = tonemap(sphericalHarmonics(n));\n\
    ambient += sh * 0.2;\n\
    diffuse += sh * 0.2;\n\
#endif\n\
\n\
#if defined(SHADOW_MAP) && defined(SHADOW_MAP_SIZE) && !defined(PLATFORM_RPI)\n\
    float bias = 0.005;\n\
    t *= clamp( 0.5 + textureShadowPCF(u_ligthShadowMap, vec2(SHADOW_MAP_SIZE), v_lightcoord.xy, v_lightcoord.z - bias), 0., 1.);\n\
#endif\n\
\n\
    color = mix(vec3(0.0, 0.0, .3) * ambient * 0.7, \n\
                diffuse, \n\
                t);\n\
\n\
    float metal = 0.0;\n\
    float roughness = 0.8;\n\
\n\
    #ifdef MATERIAL_METALLIC\n\
    metal = MATERIAL_METALLIC;\n\
    #endif\n\
\n\
    #ifdef MATERIAL_ROUGHNESS\n\
    roughness = MATERIAL_ROUGHNESS;\n\
    #endif\n\
\n\
    float notMetal = 1.0 - metal;\n\
    float smooth = .95 - saturate( roughness * (1.0-metal) );\n\
    smooth *= smooth;\n\
    smooth *= smooth;\n\
\n\
    vec3 ambientSpecular = specular;\n\
    vec3 r = reflect(-e, n);\n\
\n\
    #ifdef MATERIAL_METALLIC\n\
    shininess = smooth * (80. + 160. * notMetal);\n\
    #endif\n\
\n\
    float specIntensity = (0.35 * notMetal + 2.0 * metal) * saturate(1.1 + dot(n, e) + metal) * (metal + smooth + 4.0);\n\
    specIntensity *= t;\n\
\n\
#ifdef CUBE_MAP\n\
    ambientSpecular *= myPow( tonemap(textureCube(u_cubeMap, r).rgb), shininess) * specIntensity;\n\
#else\n\
    vec3  rAbs = abs(r);\n\
    ambientSpecular *= myPow(max(max(rAbs.x, rAbs.y), rAbs.z) + 0.005, shininess * 5.0) * specIntensity;\n\
#endif\n\
\n\
    color.rgb = color.rgb * notMetal + ambientSpecular * (notMetal * smooth + color.rgb * metal);\n\
\n\
    color += emission;\n\
\n\
#endif\n\
\n\
    gl_FragColor = vec4(color, 1.0);\n\
}\n";
