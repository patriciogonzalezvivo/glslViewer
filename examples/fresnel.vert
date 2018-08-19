#ifdef GL_ES
precision mediump float;
#endif

// By Karim Naaji
// https://github.com/karimnaaji/hdreffects/blob/master/build/shaders/fresnel.vert

uniform mat4 u_modelViewProjectionMatrix;
uniform vec3 u_eye;

attribute vec4 a_position;
attribute vec4 a_color;
attribute vec3 a_normal;
attribute vec2 a_texcoord;

varying vec4 v_position;
varying vec4 v_color;
varying vec3 v_normal;
varying vec2 v_texcoord;

const float etaR = 0.64;
const float etaG = 0.65;
const float etaB = 0.66;
const float fresnelPower = 6.0;

const float f = ((1.0-etaG)*(1.0-etaG)) / ((1.0+etaG)*(1.0+etaG));

varying vec3 v_reflect;
varying vec3 v_refractR;
varying vec3 v_refractG;
varying vec3 v_refractB;
varying float v_ratio;

void main(void) {
    v_position = a_position;
    v_color = a_color;
    v_normal = a_normal;
    v_texcoord = a_texcoord;
    gl_Position = u_modelViewProjectionMatrix * v_position;

    vec3 i = normalize(v_position.xyz - u_eye);

    v_ratio = f + (1.0 - f) * pow((1.0 - dot(-i, v_normal)), fresnelPower);

    v_refractR = refract(i, v_normal, etaR);
    v_refractG = refract(i, v_normal, etaG);
    v_refractB = refract(i, v_normal, etaB);

    v_reflect = reflect(i, v_normal);
}
