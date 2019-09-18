#ifdef GL_ES
precision mediump float;
#endif

uniform mat4 u_modelViewProjectionMatrix;

uniform float   u_time;
uniform vec2    u_resolution;

varying vec4 v_position;
varying vec4 v_color;
varying vec3 v_normal;
varying vec2 v_texcoord;

void main(void) {
    vec3 color = vec3(1.0);
    vec2 st = gl_FragCoord.xy/u_resolution.xy;
    vec2 uv = v_texcoord;

    color = v_color.rgb;

    float shade = dot(v_normal, normalize(vec3(0.0, 0.75, 0.75)));
    color *= smoothstep(-1.0, 1.0, shade);

    color.rgb *= step(fract((st.x - st.y - u_time * 0.1) * 40.0), shade * 0.5);
    
    gl_FragColor = vec4(color, 1.0);
}
