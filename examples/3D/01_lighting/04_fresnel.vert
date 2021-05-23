#ifdef GL_ES
precision mediump float;
#endif

// By Karim Naaji
// https://github.com/karimnaaji/hdreffects/blob/master/build/shaders/fresnel.vert

uniform mat4    u_modelMatrix;
uniform mat4    u_viewMatrix;
uniform mat4    u_projectionMatrix;
uniform mat4    u_modelViewProjectionMatrix;
uniform mat3    u_normalMatrix;
uniform vec3    u_camera;

attribute vec4  a_position;
varying vec4    v_position;

#ifdef MODEL_VERTEX_COLOR
attribute vec4  a_color;
varying vec4    v_color;
#endif

#ifdef MODEL_VERTEX_NORMAL
attribute vec3  a_normal;
varying vec3    v_normal;
#endif

#ifdef MODEL_VERTEX_TEXCOORD
attribute vec2  a_texcoord;
varying vec2    v_texcoord;
#endif

const float     etaR = 0.64;
const float     etaG = 0.65;
const float     etaB = 0.66;
const float     fresnelPower = 6.0;

const float f = ((1.0-etaG)*(1.0-etaG)) / ((1.0+etaG)*(1.0+etaG));

varying vec3    v_reflect;
varying vec3    v_refractR;
varying vec3    v_refractG;
varying vec3    v_refractB;
varying float   v_ratio;

void main(void) {
    v_position = a_position;

    #ifdef MODEL_VERTEX_COLOR
    v_color = a_color;
    #endif

    #ifdef MODEL_VERTEX_NORMAL
    v_normal = a_normal;
    #endif

    #ifdef MODEL_VERTEX_TEXCOORD
    v_texcoord = a_texcoord;
    #endif

    vec3 i = normalize(v_position.xyz - u_camera);
    v_ratio = f + (1.0 - f) * pow((1.0 - dot(-i, v_normal)), fresnelPower);

    v_refractR = refract(i, v_normal, etaR);
    v_refractG = refract(i, v_normal, etaG);
    v_refractB = refract(i, v_normal, etaB);

    v_reflect = reflect(i, v_normal);

    gl_Position = u_modelViewProjectionMatrix * v_position;
}
