#pragma once

#include <string>


// DEFAULT SHADERS
// -----------------------------------------------------
std::string default_vert = "\n\
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

std::string default_frag = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
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
varying vec4    v_tangent;\n\
#endif\n\
\n\
void main(void) {\n\
    vec3 color = vec3(1.0);\n\
    \n\
    #ifdef MODEL_HAS_COLORS\n\
    color = v_color.rgb;\n\
    #endif\n\
    \n\
    vec3 l = normalize(u_light);\n\
    vec3 n = normalize(v_normal);\n\
    vec3 v = normalize(u_camera);\n\
    vec3 h = normalize(l + v);\n\
    \n\
    float t = dot(n, l) * 0.5 + 0.5;\n\
    float s = max(0.0, dot(n, h));\n\
    s = pow(s, 20.0);\n\
    \n\
    // Gooch Light Model (w Blinn specular)\n\
    color = mix(mix(vec3(0.0, 0.0, .3) + color * 0.255, \n\
                    vec3(0.35, 0.25, 0.0) + color * 0.255, \n\
                    t), \n\
                vec3(1.0, 1.0, 1.0), \n\
                s);\n\
    \n\
    gl_FragColor = vec4(color, 1.0);\n\
}\n\
";

// 2D
// -----------------------------------------------------

std::string billboard_vert = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
attribute vec4 a_position;\n\
attribute vec2 a_texcoord;\n\
\n\
varying vec4 v_position;\n\
varying vec4 v_color;\n\
varying vec3 v_normal;\n\
varying vec2 v_texcoord;\n\
\n\
#ifdef SHADOW_MAP\n\
uniform mat4    u_lightMatrix;\n\
varying vec4    v_lightcoord;\n\
#endif\n\
\n\
void main(void) {\n\
    v_position =  a_position;\n\
    v_color = vec4(1.0);\n\
    v_normal = vec3(0.0,0.0,1.0);\n\
    v_texcoord = a_texcoord;\n\
    \n\
#ifdef SHADOW_MAP\n\
    v_lightcoord = u_lightMatrix * v_position;\n\
#endif\n\
    \n\
    gl_Position = v_position;\n\
}";

std::string dynamic_billboard_vert = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
uniform mat4 u_modelViewProjectionMatrix;\n\
uniform vec2 u_translate;\n\
uniform vec2 u_scale;\n\
attribute vec4 a_position;\n\
attribute vec2 a_texcoord;\n\
varying vec2 v_texcoord;\n\
\n\
void main(void) {\n\
    vec4 position = a_position;\n\
    position.xy *= u_scale;\n\
    position.xy += u_translate;\n\
    v_texcoord = a_texcoord;\n\
    gl_Position = u_modelViewProjectionMatrix * position;\n\
}";

std::string dynamic_billboard_frag = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
uniform sampler2D u_tex0;\n\
uniform vec4 u_color;\n\
uniform float u_depth;\n\
uniform float u_cameraNearClip;\n\
uniform float u_cameraFarClip;\n\
uniform float u_cameraDistance;\n\
\n\
varying vec2 v_texcoord;\n\
\n\
float linearizeDepth(float zoverw) {\n\
	return (2.0 * u_cameraNearClip) / (u_cameraFarClip + u_cameraNearClip - zoverw * (u_cameraFarClip - u_cameraNearClip));\n\
}\n\
\n\
vec3 heatmap(float v) {\n\
    vec3 r = v * 2.1 - vec3(1.8, 1.14, 0.3);\n\
    return 1.0 - r * r;\n\
}\n\
\n\
void main(void) { \n\
    vec4 color = u_color;\n\
    color += texture2D(u_tex0, v_texcoord);\n\
    \n\
    if (u_depth > 0.0) {\n\
        color.r = linearizeDepth(color.r) * u_cameraFarClip;\n\
        color.rgb = heatmap(1.0 - (color.r - u_cameraDistance) * 0.01);\n\
    }\n\
    \n\
    gl_FragColor = color;\n\
}\n";

std::string wireframe2D_vert = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
uniform mat4 u_modelViewProjectionMatrix;\n\
uniform vec2 u_translate;\n\
uniform vec2 u_scale;\n\
attribute vec4 a_position;\n\
\n\
void main(void) {\n\
    vec4 position = a_position;\n\
    position.xy *= u_scale;\n\
    position.xy += u_translate;\n\
    gl_Position = u_modelViewProjectionMatrix * position;\n\
}\n";

std::string wireframe2D_frag = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
uniform vec4 u_color;\n\
\n\
void main(void) { \n\
    gl_FragColor = u_color;\n\
}\n";
