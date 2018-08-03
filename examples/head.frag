#ifdef GL_ES
precision mediump float;
#endif

uniform mat4 u_modelViewProjectionMatrix;
uniform mat4 u_modelMatrix;
uniform mat4 u_viewMatrix;
uniform mat4 u_projectionMatrix;
uniform mat4 u_normalMatrix;

uniform vec4 u_date;
uniform vec3 u_centre3d;
uniform vec3 u_eye3d;
uniform vec3 u_up3d;
uniform vec3 u_view2d;
uniform vec2 u_resolution;
uniform vec2 u_mouse;
uniform float u_time;
uniform float u_delta;

varying vec4 v_position;
varying vec4 v_color;
varying vec3 v_normal;
varying vec2 v_texcoord;

void main(void) {
    vec3 color = vec3(0.0);
    vec3 normal = v_normal;
    vec3 pos = v_position.xyz;
    vec2 st = v_texcoord;
    
    color = v_color.rgb;
    // color.rgb = v_normal * 0.5 + 0.5;
    // color.xy += st.xy;

    // color *= step(.5, fract(pos.z * 2. + u_time));
    color *= step( 1.0 - smoothstep(0.2, 0.8, color.r) * 0.5 , fract(st.x + st.y * 2. + u_time));

    gl_FragColor = vec4(color, 1.0);
}

