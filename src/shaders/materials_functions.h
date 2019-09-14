#pragma once

#include <string>

const std::string materials_functions = "\n\
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
#ifdef MATERIAL_DIFFUSEMAP\n\
uniform sampler2D MATERIAL_DIFFUSEMAP;\n\
#endif\n\
\n\
vec3 getMaterialAlbedo() {\n\
    vec3 base = vec3(0.776, 0.754, 0.74);\n\
#if defined(MATERIAL_DIFFUSEMAP) && defined(MODEL_HAS_TEXCOORDS)\n\
    base *= texture2D(MATERIAL_DIFFUSEMAP, v_texcoord.xy).rgb;\n\
#elif defined(MATERIAL_DIFFUSE)\n\
    base = MATERIAL_DIFFUSE;\n\
#endif\n\
\n\
#if defined(MODEL_HAS_COLORS)\n\
    base *= v_color.rgb;\n\
#endif\n\
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
    emission *= texture2D(MATERIAL_EMISSIVEMAP, v_texcoord.xy).rgb;\n\
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
float getMaterialMetallic() {\n\
    float metallic = 0.0;\n\
#if defined(MATERIAL_METALLICMAP) && defined(MODEL_HAS_TEXCOORDS)\n\
    metallic = texture2D(MATERIAL_METALLICMAP, v_texcoord.xy).r;\n\
#elif defined(MATERIAL_METALLIC)\n\
    metallic = MATERIAL_METALLIC;\n\
#elif defined(DEBUG)\n\
    metallic = checkBoard(vec2(0.0, 4.0)) * 0.5;\n\
#endif\n\
    return metallic;\n\
}\n\
\n\
#ifdef MATERIAL_ROUGHNESSMAP\n\
uniform sampler2D MATERIAL_ROUGHNESSMAP;\n\
#endif\n\
\n\
float getMaterialRoughness() {\n\
    float roughness = 0.2;\n\
#if defined(MATERIAL_ROUGHNESSMAP) && defined(MODEL_HAS_TEXCOORDS)\n\
    roughness = texture2D(MATERIAL_ROUGHNESSMAP, v_texcoord.xy).r;\n\
#elif defined(MATERIAL_ROUGHNESS)\n\
    roughness = MATERIAL_ROUGHNESS;\n\
#elif defined(DEBUG)\n\
    roughness = checkBoard(vec2(4.0, 0.0));\n\
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
\n";
