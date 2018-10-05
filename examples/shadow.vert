#ifdef GL_ES
precision mediump float;
#endif

uniform mat4    u_modelViewProjectionMatrix;
uniform mat4    u_viewMatrix;
uniform mat4    u_projectionMatrix;

attribute vec4  a_position;
attribute vec4  a_color;
attribute vec3  a_normal;
attribute vec2  a_texcoord;

varying vec4    v_position;
varying vec3    v_normal;
varying vec2    v_texcoord;

#ifdef MODEL_HAS_COLORS
varying vec4    v_color;
#endif

#ifdef MODEL_HAS_TANGENTS
attribute vec4  a_tangent;
varying vec4    v_tangent;
varying mat3    v_tangentToWorld;
#endif

uniform mat4    u_lightMatrix;
varying vec4    v_lightcoord;

void main(void) {
    v_position = a_position;
    v_normal = a_normal;
    v_texcoord = a_texcoord;

#ifdef MODEL_HAS_COLORS
    v_color = a_color;
#endif

#ifdef MODEL_HAS_TANGENTS
    v_tangent = a_tangent;
    vec3 worldTangent = a_tangent.xyz;
    vec3 worldBiTangent = cross(v_normal, worldTangent) * sign(a_tangent.w);
    v_tangentToWorld = mat3(normalize(worldTangent), normalize(worldBiTangent), normalize(v_normal));
#endif

    v_lightcoord = u_lightMatrix * v_position;
    gl_Position = u_modelViewProjectionMatrix * v_position;
}
