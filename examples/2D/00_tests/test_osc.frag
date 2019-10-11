#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_resolution;
// uniform vec2 u_mouse;
// uniform float u_time;

uniform float multifaderM_1;
uniform float multifaderM_2;
uniform float multifaderM_3;

varying vec4 v_position;
varying vec4 v_color;
varying vec3 v_normal;
varying vec2 v_texcoord;

void main (void) {
    vec2 st = gl_FragCoord.xy/u_resolution.xy;
    float aspect = u_resolution.x/u_resolution.y;
    st.x *= aspect;

    vec3 color = vec3(0.0);
    color.r = multifaderM_1;
    color.g = multifaderM_2;
    color.b = multifaderM_3;
    
    gl_FragColor = vec4(color,1.0);
}
