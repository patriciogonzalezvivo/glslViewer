#ifdef GL_ES
precision mediump float;
#endif

uniform vec3        u_camera;
uniform vec3        u_light;
uniform vec2        u_resolution;

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

#ifdef MODEL_VERTEX_TEXCOORDS
varying vec2        v_texcoord;
#endif

#include "sh.glsl"

void main(void) {
    vec3 color = vec3(0.5);

    #ifdef MODEL_VERTEX_COLOR
    color *= v_color.rgb;
    #endif

    vec3 n = normalize(v_normal);
    vec3 l = normalize(u_light);
    vec3 v = normalize(u_camera - v_position.xyz);

    float diffuse = (dot(n, l) + 1.0 ) * 0.5;
    color *= diffuse;

    // Computing Spherical Harmonics
    vec3 sh = sh(n) * 0.5;

    // add Spherical Harmonics to left side
    color *= mix(vec3(1.0), sh, 1.0-step(-1., v_position.x));

    gl_FragColor = vec4(color, 1.0);
}
