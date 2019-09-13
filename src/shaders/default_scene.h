#pragma once

#include "default_header.h"
#include "materials_functions.h"
#include "ibl_functions.h"
#include "shadows_functions.h"
#include "light_functions.h"
#include "calc_functions.h"

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

const std::string default_scene_frag = default_header + materials_functions + ibl_functions + shadows_functions + light_functions + calc_functions + "\n\
void main(void) {\n\
    vec3 baseColor = getBaseColor();\n\
    vec3 emissionColor = getEmissionColor();\n\
    float roughness = getRoughness();\n\
    float metallic = getMetallic();\n\
    float occlusion = 1.0;\n\
\n\
    // Set Normal and Reflection\n\
    vec3 normal = vec3(0.0);\n\
    vec3 reflectDir = vec3(0.0);\n\
    vec3 viewDir = normalize(u_camera);\n\
    calcNormal(viewDir, normal, reflectDir);\n\
\n\
    // Set Color\n\
    vec3 color = calcColor(baseColor, normal, viewDir, reflectDir, roughness, metallic);\n\
    color = max(color, emissionColor);\n\
\n\
    gl_FragColor = vec4(color, 1.0);\n\
}\n";
