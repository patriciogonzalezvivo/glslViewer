
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_buffer0;

uniform mat4        u_modelMatrix;
uniform mat4        u_viewMatrix;
uniform mat4        u_projectionMatrix;
uniform mat4        u_modelViewProjectionMatrix;
uniform float       u_time;

attribute vec4      a_position;
varying vec4        v_position;

varying vec4        v_color;
varying vec2        v_texcoord;

void main(void) {
    v_position = a_position;
    v_texcoord = a_position.xy;

    float scale = 50.0;

    v_position.xyz = texture2D(u_buffer0, v_texcoord).xyz;
    // v_color.rgb = v_position.xyz;
    v_color.rgb = vec3(v_texcoord, a_position.z);

    v_position.xyz = fract(v_position.xyz);
    v_position.xyz = (v_position.xyz - 0.5) * scale;

    float brightness = 1.0-length(v_position.xyz)/scale;
    brightness = pow(brightness, 16.0);
    v_color.a = brightness;
    gl_PointSize = 5.0 * brightness;
    
    gl_Position = u_projectionMatrix * u_viewMatrix * v_position;
}
