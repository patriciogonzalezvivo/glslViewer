
#ifdef GL_ES
precision highp float;
#endif

// Copyright Patricio Gonzalez Vivo, 2021 - http://patriciogonzalezvivo.com/
// I am the sole copyright owner of this Work.
//
// You cannot host, display, distribute or share this Work in any form,
// including physical and digital. You cannot use this Work in any
// commercial or non-commercial product, website or project. You cannot
// sell this Work and you cannot mint an NFTs of it.
// I share this Work for educational purposes, and you can link to it,
// through an URL, proper attribution and unmodified screenshot, as part
// of your educational material. If these conditions are too restrictive
// please contact me and we'll definitely work it out.


uniform sampler2D   u_doubleBuffer0;  // pos

uniform mat4        u_modelMatrix;
uniform mat4        u_viewMatrix;
uniform mat4        u_projectionMatrix;
uniform mat4        u_modelViewProjectionMatrix;
uniform vec3        u_camera;
uniform vec2        u_resolution;
uniform float       u_time;

attribute vec4      a_position;
varying vec4        v_position;

varying vec4        v_color;
varying vec2        v_texcoord;

float population = 0.01; // 0 = none / 1 = 100%
#define decimation(value, presicion) (floor(value * presicion)/presicion)

void main(void) {
    v_position = a_position;
    v_texcoord = a_position.xy;

    vec2 uv = v_texcoord;

    vec2 buffRes = vec2(512.0);
    vec2 buffPixel = 1.0/buffRes;
    uv = decimation(uv, buffRes) + 0.5 * buffPixel;
    v_texcoord = uv;

    vec4 data = texture2D(u_doubleBuffer0, uv);
    v_position.xy = data.xy;

    float visible = step(a_position.z, population);
    v_color = vec4(data.z, visible, data.w, 1.0);
    // gl_PointSize = visible;
    
    
    gl_Position = v_position;
}
