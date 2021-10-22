#ifdef GL_ES
precision mediump float;
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

uniform sampler2D   u_buffer0;  // pos
uniform sampler2D   u_buffer2;  // vel 

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

#define decimation(value, presicion) (floor(value * presicion)/presicion)

void main(void) {
    v_position = a_position;
    v_texcoord = a_position.xy;
    vec2 pixel = 1.0/u_resolution;

    float scale = 10.0;
    // scale = 5.0+mix(5.0, 100., cos(u_time * 0.1) * 0.5 + 0.5);

    vec2 uv = v_texcoord;
    uv = decimation(uv, u_resolution) + 0.5 * pixel;

    v_position.xyz = texture2D(u_buffer0, uv).xyz * 2.0 - 1.0;
    v_color.rgb =  texture2D(u_buffer2, uv).xyz * 2.0 - 1.0;
    v_color.rgb =  normalize(v_color.xyz) * 0.5 + 0.5;
    v_position.xyz *= scale;

    float brightness = 1.0- max( 0.0, distance(u_camera, v_position.xyz)/ (scale * 2.0));
    // v_color.a = pow(brightness, 1.0);
    gl_PointSize = 10.0 * pow(brightness, 10.0);
    
    gl_Position = u_projectionMatrix * u_viewMatrix * v_position;
}
