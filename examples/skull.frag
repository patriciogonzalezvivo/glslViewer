#ifdef GL_ES
precision mediump float;
#endif

uniform mat4 u_modelViewProjectionMatrix;
uniform mat4 u_modelMatrix;
uniform mat4 u_viewMatrix;
uniform mat4 u_projectionMatrix;
uniform mat4 u_normalMatrix;

uniform float u_time;
uniform vec2 u_mouse;
uniform vec2 u_resolution;

varying vec4 v_position;
varying vec4 v_color;
varying vec3 v_normal;
varying vec2 v_texcoord;

void main(void) {
    vec3 color = vec3(0.0);
    color = v_color.rgb;
    vec3 normal = v_normal;
    vec3 pos = v_position.xyz;
    vec2 uv = v_texcoord;

    color -= normal * .5 + .5;

    color *= step(.5, fract(pos.z * 2. + u_time));
    // color *= step(.5, fract(uv.y * 5. + u_time));

    gl_FragColor = vec4(color,1.0);
}

