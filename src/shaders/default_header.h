#pragma once

#include <string>

const std::string default_header = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
uniform vec3        u_light;\n\
uniform vec3        u_lightColor;\n\
uniform vec3        u_camera;\n\
uniform vec2        u_resolution;\n\
\n\
varying vec4        v_position;\n\
\n\
#ifdef MODEL_HAS_COLORS\n\
varying vec4        v_color;\n\
#endif\n\
\n\
#ifdef MODEL_HAS_NORMALS\n\
varying vec3        v_normal;\n\
#endif\n\
\n\
#ifdef MODEL_HAS_TEXCOORDS\n\
varying vec2        v_texcoord;\n\
#endif\n\
\n\
#ifdef MODEL_HAS_TANGENTS\n\
varying mat3        v_tangentToWorld;\n\
varying vec4        v_tangent;\n\
#endif\n\
\n";