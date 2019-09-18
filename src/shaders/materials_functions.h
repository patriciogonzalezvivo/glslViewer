#pragma once

#include <string>

const std::string materials_functions = "\n\
#ifndef FNC_GETMATERIAL\n\
\n\
float checkBoard(vec2 _scale) {\n\
#ifdef MODEL_VERTEX_TEXCOORD\n\
    vec2 uv = v_texcoord;\n\
#else\n\
    vec2 uv = gl_FragCoord.xy/u_resolution;\n\
#endif\n\
    uv = floor(fract(uv * _scale) * 2.0);\n\
    return min(1.0, uv.x + uv.y) - (uv.x * uv.y);\n\
}\n\
\n\
const float GAMMA = 2.2;\n\
const float INV_GAMMA = 1.0 / GAMMA;\n\
\n\
// see http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html\n\
vec3 LINEARtoSRGB(vec3 color) { return pow(color, vec3(INV_GAMMA)); }\n\
// vec4 SRGBtoLINEAR(vec4 srgbIn) { return vec4(pow(srgbIn.xyz, vec3(GAMMA)), srgbIn.w); }\n\
vec4 SRGBtoLINEAR(vec4 srgbIn) { return vec4(srgbIn.xyz * srgbIn.xyz, srgbIn.w); }\n\
\n\
#ifdef MATERIAL_ALBEDOMAP\n\
uniform sampler2D MATERIAL_ALBEDOMAP;\n\
#endif\n\
\n\
vec3 getMaterialAlbedo() {\n\
    vec3 base = vec3(1.0);\n\
#if defined(MATERIAL_ALBEDOMAP) && defined(MODEL_VERTEX_TEXCOORD)\n\
    vec2 uv = v_texcoord.xy;\n\
    #if defined(MATERIAL_ALBEDOMAP_OFFSET)\n\
    uv += (MATERIAL_ALBEDOMAP_OFFSET).xy;\n\
    #endif\n\
    #if defined(MATERIAL_ALBEDOMAP_SCALE)\n\
    uv *= (MATERIAL_ALBEDOMAP_SCALE).xy;\n\
    #endif\n\
    base = SRGBtoLINEAR(texture2D(MATERIAL_ALBEDOMAP, uv)).rgb;\n\
#elif defined(MATERIAL_ALBEDO)\n\
    base = MATERIAL_ALBEDO;\n\
#elif !defined(MODEL_VERTEX_COLOR)\n\
    base *= 0.5 + checkBoard(vec2(8.0)) * 0.5;\n\
#endif\n\
\n\
#if defined(MODEL_VERTEX_COLOR)\n\
    base *= v_color.rgb;\n\
#endif\n\
    return base;\n\
}\n\
\n\
#ifdef MATERIAL_SPECULARMAP\n\
uniform sampler2D MATERIAL_SPECULARMAP;\n\
#endif\n\
\n\
vec3 getMaterialSpecular() {\n\
    vec3 spec = vec3(0.04);\n\
#if defined(MATERIAL_SPECULARMAP) && defined(MODEL_VERTEX_TEXCOORD)\n\
    vec2 uv = v_texcoord.xy;\n\
    #if defined(MATERIAL_SPECULARMAP_OFFSET)\n\
    uv += (MATERIAL_SPECULARMAP_OFFSET).xy;\n\
    #endif\n\
    #if defined(MATERIAL_SPECULARMAP_SCALE)\n\
    uv *= (MATERIAL_SPECULARMAP_SCALE).xy;\n\
    #endif\n\
    spec = SRGBtoLINEAR(texture2D(MATERIAL_SPECULARMAP, uv)).rgb;\n\
#elif defined(MATERIAL_SPECULAR)\n\
    spec = MATERIAL_SPECULAR;\n\
#endif\n\
    return spec;\n\
}\n\
\n\
#ifdef MATERIAL_EMISSIONMAP\n\
uniform sampler2D MATERIAL_EMISSIONMAP;\n\
#endif\n\
\n\
vec3 getMaterialEmission() {\n\
    vec3 emission = vec3(0.0);\n\
#if defined(MATERIAL_EMISSIONMAP) && defined(MODEL_VERTEX_TEXCOORD)\n\
    vec2 uv = v_texcoord.xy;\n\
    #if defined(MATERIAL_EMISSIONMAP_OFFSET)\n\
    uv += (MATERIAL_EMISSIONMAP_OFFSET).xy;\n\
    #endif\n\
    #if defined(MATERIAL_EMISSIONMAP_SCALE)\n\
    uv *= (MATERIAL_EMISSIONMAP_SCALE).xy;\n\
    #endif\n\
    emission = SRGBtoLINEAR(texture2D(MATERIAL_EMISSIONMAP, uv)).rgb;\n\
#elif defined(MATERIAL_EMISSION)\n\
    emission = MATERIAL_EMISSION;\n\
#endif\n\
    return emission;\n\
}\n\
\n\
#ifdef MATERIAL_NORMALMAP\n\
uniform sampler2D MATERIAL_NORMALMAP;\n\
#endif\n\
\n\
#ifdef MATERIAL_BUMPMAP_NORMALMAP\n\
uniform sampler2D MATERIAL_BUMPMAP_NORMALMAP;\n\
#endif\n\
\n\
vec3 getMaterialNormal() {\n\
    vec3 normal = vec3(0.0, 0.0, 1.0);\n\
#ifdef MODEL_VERTEX_NORMAL\n\
    normal = v_normal;\n\
    #if defined(MODEL_VERTEX_TANGENT) && defined(MODEL_VERTEX_TEXCOORD) && defined(MATERIAL_NORMALMAP)\n\
    vec2 uv = v_texcoord.xy;\n\
        #if defined(MATERIAL_NORMALMAP_OFFSET)\n\
    uv += (MATERIAL_NORMALMAP_OFFSET).xy;\n\
        #endif\n\
        #if defined(MATERIAL_NORMALMAP_SCALE)\n\
    uv *= (MATERIAL_NORMALMAP_SCALE).xy;\n\
        #endif\n\
    normal = texture2D(MATERIAL_NORMALMAP, uv).xyz;\n\
    normal = v_tangentToWorld * (normal * 2.0 - 1.0);\n\
    #elif defined(MODEL_VERTEX_TANGENT) && defined(MODEL_VERTEX_TEXCOORD) && defined(MATERIAL_BUMPMAP_NORMALMAP)\n\
    vec2 uv = v_texcoord.xy;\n\
        #if defined(MATERIAL_BUMPMAP_OFFSET)\n\
    uv += (MATERIAL_BUMPMAP_OFFSET).xy;\n\
        #endif\n\
        #if defined(MATERIAL_BUMPMAP_SCALE)\n\
    uv *= (MATERIAL_BUMPMAP_SCALE).xy;\n\
        #endif\n\
    normal = v_tangentToWorld * (texture2D(MATERIAL_BUMPMAP_NORMALMAP, uv).xyz * 2.0 - 1.0);\n\
    #endif\n\
#endif\n\
    return normal;\n\
}\n\
\n\
\n\
#ifdef MATERIAL_ROUGHNESSMAP\n\
uniform sampler2D MATERIAL_ROUGHNESSMAP;\n\
#endif\n\
\n\
const float c_MinReflectance = 0.04;\n\
// Gets metallic factor from specular glossiness workflow inputs \n\
float convertMetallic(vec3 diffuse, vec3 specular, float maxSpecular) {\n\
    float perceivedDiffuse = sqrt(0.299 * diffuse.r * diffuse.r + 0.587 * diffuse.g * diffuse.g + 0.114 * diffuse.b * diffuse.b);\n\
    float perceivedSpecular = sqrt(0.299 * specular.r * specular.r + 0.587 * specular.g * specular.g + 0.114 * specular.b * specular.b);\n\
    if (perceivedSpecular < c_MinReflectance) {\n\
        return 0.0;\n\
    }\n\
    float a = c_MinReflectance;\n\
    float b = perceivedDiffuse * (1.0 - maxSpecular) / (1.0 - c_MinReflectance) + perceivedSpecular - 2.0 * c_MinReflectance;\n\
    float c = c_MinReflectance - perceivedSpecular;\n\
    float D = max(b * b - 4.0 * a * c, 0.0);\n\
    return clamp((-b + sqrt(D)) / (2.0 * a), 0.0, 1.0);\n\
}\n\
\n\
float getMaterialMetallic() {\n\
    float metallic = 0.0;\n\
#if defined(MATERIAL_METALLICMAP) && defined(MODEL_VERTEX_TEXCOORD)\n\
    vec2 uv = v_texcoord.xy;\n\
    #if defined(MATERIAL_METALLICMAP_OFFSET)\n\
    uv += (MATERIAL_METALLICMAP_OFFSET).xy;\n\
    #endif\n\
    #if defined(MATERIAL_METALLICMAP_SCALE)\n\
    uv *= (MATERIAL_METALLICMAP_SCALE).xy;\n\
    #endif\n\
    metallic = SRGBtoLINEAR(texture2D(MATERIAL_METALLICMAP, uv)).r;\n\
#elif defined(MATERIAL_METALLIC)\n\
    metallic = MATERIAL_METALLIC;\n\
#else\n\
    vec3 diffuse = getMaterialAlbedo();\n\
    vec3 specular = getMaterialSpecular();\n\
    float maxSpecula = max(max(specular.r, specular.g), specular.b);\n\
    metallic = convertMetallic(diffuse, specular, maxSpecula);\n\
#endif\n\
    return metallic;\n\
}\n\
\n\
\n\
#ifdef MATERIAL_ROUGHNESSMAP\n\
uniform sampler2D MATERIAL_ROUGHNESSMAP;\n\
#endif\n\
\n\
float getMaterialRoughness() {\n\
    float roughness = 0.1;\n\
#if defined(MATERIAL_ROUGHNESSMAP) && defined(MODEL_VERTEX_TEXCOORD)\n\
    vec2 uv = v_texcoord.xy;\n\
    #if defined(MATERIAL_ROUGHNESSMAP_OFFSET)\n\
    uv += (MATERIAL_ROUGHNESSMAP_OFFSET).xy;\n\
    #endif\n\
    #if defined(MATERIAL_ROUGHNESSMAP_SCALE)\n\
    uv *= (MATERIAL_ROUGHNESSMAP_SCALE).xy;\n\
    #endif\n\
    roughness = SRGBtoLINEAR(texture2D(MATERIAL_ROUGHNESSMAP, uv)).r;\n\
#elif defined(MATERIAL_ROUGHNESS)\n\
    roughness = MATERIAL_ROUGHNESS;\n\
#elif defined(DEBUG)\n\
    roughness = checkBoard(vec2(20.0, 0.0)) * 0.25;\n\
#endif\n\
    return roughness;\n\
}\n\
\n\
float getMaterialShininess() {\n\
    float shininess = 15.0;\n\
#ifdef MATERIAL_SHININESS\n\
    shininess = MATERIAL_SHININESS;\n\
#elif defined(MATERIAL_METALLIC) && defined(MATERIAL_ROUGHNESS)\n\
    float smooth = .95 - MATERIAL_ROUGHNESS * 0.5;\n\
    smooth *= smooth;\n\
    smooth *= smooth;\n\
    shininess = smooth * (80. + 160. * (1.0-MATERIAL_METALLIC));\n\
#endif\n\
    return shininess;\n\
}\n\
\n\
#endif\n\
\n";
