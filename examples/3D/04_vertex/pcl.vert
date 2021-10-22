#ifdef GL_ES
precision mediump float;
#endif

// Copyright Patricio Gonzalez Vivo, 2016 - http://patriciogonzalezvivo.com/
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

uniform sampler2D   u_tex0;
uniform vec2        u_tex0Resolution;
uniform sampler2D   u_tex0Depth;
uniform vec2        u_tex0DepthResolution;

uniform mat4        u_modelViewProjectionMatrix;

attribute vec4      a_position;
varying vec4        v_position;

varying vec4        v_color;
varying vec2        v_texcoord;

void main(void) {
    v_position = a_position;

    v_texcoord = v_position.xy;

    vec2 st = v_texcoord;
    v_position.x *= u_tex0Resolution.x/u_tex0Resolution.y;
    v_color = texture2D(u_tex0, st);
    float depth = texture2D(u_tex0Depth, st).r;
    v_position.z = depth;
    
#ifdef MODEL_VERTEX_NORMAL
    v_normal = a_normal;
#endif
    
#ifdef MODEL_VERTEX_TEXCOORD
    v_texcoord = a_texcoord;
#endif

    gl_PointSize = 2.0;
    
    gl_Position = u_modelViewProjectionMatrix * v_position;
}
