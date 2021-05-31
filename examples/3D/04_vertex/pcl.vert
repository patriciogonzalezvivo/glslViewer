
#ifdef GL_ES
precision mediump float;
#endif

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
