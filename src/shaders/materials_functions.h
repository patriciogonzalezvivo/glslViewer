#pragma once

#include <string>

const std::string materials_functions = "\n\
#ifndef FNC_GETMATERIAL\n\
\n\
float checkBoard(vec2 _scale) {\n\
#ifdef MODEL_HAS_TEXCOORDS\n\
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
vec4 SRGBtoLINEAR(vec4 srgbIn) { return vec4(pow(srgbIn.xyz, vec3(GAMMA)), srgbIn.w); }\n\
\n\
#ifdef MATERIAL_DIFFUSEMAP\n\
uniform sampler2D MATERIAL_DIFFUSEMAP;\n\
#endif\n\
\n\
vec3 getMaterialAlbedo() {\n\
    vec3 base = vec3(1.0);\n\
\n\
#if defined(MATERIAL_DIFFUSEMAP) && defined(MODEL_HAS_TEXCOORDS)\n\
    base = SRGBtoLINEAR(texture2D(MATERIAL_DIFFUSEMAP, v_texcoord.xy)).rgb;\n\
#elif defined(MATERIAL_DIFFUSE)\n\
    base = MATERIAL_DIFFUSE;\n\
#elif !defined(MODEL_HAS_COLORS)\n\
    base *= 0.5 + checkBoard(vec2(8.0)) * 0.5;\n\
#endif\n\
\n\
#if defined(MODEL_HAS_COLORS)\n\
    base *= v_color.rgb;\n\
#endif\n\
    return base;\n\
}\n\
\n\
#ifdef MATERIAL_DIFFUSEMAP\n\
uniform sampler2D MATERIAL_SPECULARMAP;\n\
#endif\n\
\n\
vec3 getMaterialSpecular() {\n\
    vec3 base = vec3(0.02);\n\
#if defined(MATERIAL_SPECULAR)\n\
    base = MATERIAL_SPECULAR;\n\
#endif\n\
#if defined(MATERIAL_SPECULARMAP) && defined(MODEL_HAS_TEXCOORDS)\n\
    base *= SRGBtoLINEAR(texture2D(MATERIAL_SPECULARMAP, v_texcoord.xy)).rgb;\n\
#endif\n\
\n\
    return base;\n\
}\n\
\n\
#ifdef MATERIAL_EMISSIVEMAP\n\
uniform sampler2D MATERIAL_EMISSIVEMAP;\n\
#endif\n\
\n\
vec3 getMaterialEmission() {\n\
    vec3 emission = vec3(0.0);\n\
#if defined(MATERIAL_EMISSIVEMAP) && defined(MODEL_HAS_TEXCOORDS)\n\
    emission *= SRGBtoLINEAR(texture2D(MATERIAL_EMISSIVEMAP, v_texcoord.xy)).rgb;\n\
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
#ifdef MODEL_HAS_NORMALS\n\
    normal = v_normal;\n\
    #if defined(MODEL_HAS_TANGENTS) && defined(MODEL_HAS_TEXCOORDS) && defined(MATERIAL_NORMALMAP)\n\
    normal = v_tangentToWorld * (texture2D(MATERIAL_BUMPMAP_NORMALMAP, v_texcoord.xy).xyz * 2.0 - 1.0);\n\
    #elif defined(MODEL_HAS_TANGENTS) && defined(MODEL_HAS_TEXCOORDS) && defined(MATERIAL_BUMPMAP_NORMALMAP)\n\
    normal = v_tangentToWorld * (texture2D(MATERIAL_BUMPMAP_NORMALMAP, v_texcoord.xy).xyz * 2.0 - 1.0);\n\
    #endif\n\
#endif\n\
    return normal;\n\
}\n\
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
#if defined(MATERIAL_METALLICMAP) && defined(MODEL_HAS_TEXCOORDS)\n\
    metallic = SRGBtoLINEAR(texture2D(MATERIAL_METALLICMAP, v_texcoord.xy)).r;\n\
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
#ifdef MATERIAL_ROUGHNESSMAP\n\
uniform sampler2D MATERIAL_ROUGHNESSMAP;\n\
#endif\n\
\n\
float getMaterialRoughness() {\n\
    float roughness = 0.1;\n\
#if defined(MATERIAL_ROUGHNESSMAP) && defined(MODEL_HAS_TEXCOORDS)\n\
    roughness = SRGBtoLINEAR(texture2D(MATERIAL_ROUGHNESSMAP, v_texcoord.xy)).r;\n\
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
#endif\n\
    return shininess;\n\
}\n\
\n\
#endif\n\
\n";
