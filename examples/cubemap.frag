#ifdef GL_ES
precision mediump float;
#endif

uniform samplerCube u_cubeMap0;

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
// varying vec4 v_color;
varying vec3 v_normal;
varying vec2 v_texcoord;

void main(void) {
   vec3 color = vec3(1.0);

    // color = v_color.rgb;
    float shade = dot(v_normal, normalize(vec3(0.0, 0.75, 0.75)));
    color *= smoothstep(-1.0, 1.0, shade);

    vec3 refle = reflect(v_position.xyz - u_eye3d, v_normal);
    vec3 refra = refract(v_position.xyz - u_eye3d, v_normal, 0.01);

    vec4 reflection = textureCube(u_cubeMap0, refra);

    color = reflection.rgb;

    gl_FragColor = vec4(color, 1.0);
}
