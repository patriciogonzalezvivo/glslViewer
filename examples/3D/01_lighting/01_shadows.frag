#ifdef GL_ES
precision mediump float;
#endif

uniform vec3        u_camera;
uniform vec3        u_light;
uniform vec2        u_resolution;
uniform float       u_time;

varying vec4        v_position;

#ifdef MODEL_VERTEX_COLOR
varying vec4        v_color;
#endif

#ifdef MODEL_VERTEX_NORMAL
varying vec3        v_normal;
#endif

#ifdef MODEL_VERTEX_TANGENT
varying vec4        v_tangent;
varying mat3        v_tangentToWorld;
#endif

#ifdef MODEL_VERTEX_TEXCOORD
varying vec2        v_texcoord;
#endif

#include "textureShadow.glsl"

void main(void) {
    vec3 color = vec3(1.0);

#ifdef MODEL_VERTEX_COLOR
    color *= v_color.rgb;
#endif

#ifdef FLOOR
    vec2 uv = v_texcoord;
    uv = floor(fract(uv * 8.0) * 2.0);
    color = vec3(0.5) + (min(1.0, uv.x + uv.y) - (uv.x * uv.y)) * 0.5;
#endif

#ifdef MODEL_VERTEX_NORMAL
    vec3 n = normalize(v_normal);
    vec3 l = normalize(u_light);
    vec3 v = normalize(u_camera - v_position.xyz);

    float diffuse = (dot(n, l) + 1.0 ) * 0.5;
    color *= diffuse;

    float bias = 0.01;
    vec3 shadowCoord = v_lightCoord.xyz / v_lightCoord.w;
    float shadow = textureShadowPCF( u_ligthShadowMap, vec2(LIGHT_SHADOWMAP_SIZE), shadowCoord.xy, shadowCoord.z - bias);

    // add Spherical Harmonics
    color *= 0.5 + shadow * 0.5;
#endif

    gl_FragColor = vec4(color, 1.0);
}
