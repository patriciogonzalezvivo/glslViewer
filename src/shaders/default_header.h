#pragma once

#include <string>

const std::string default_header = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform vec3        u_light;
uniform vec3        u_lightColor;
uniform vec3        u_camera;
uniform vec2        u_resolution;

varying vec4        v_position;

#ifdef MODEL_VERTEX_COLOR
varying vec4        v_color;
#endif

#ifdef MODEL_VERTEX_NORMAL
varying vec3        v_normal;
#endif

#ifdef MODEL_VERTEX_TEXCOORD
varying vec2        v_texcoord;
#endif

#ifdef MODEL_VERTEX_TANGENT
varying mat3        v_tangentToWorld;
varying vec4        v_tangent;
#endif
)";
