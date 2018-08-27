#ifdef GL_ES
precision mediump float;
#endif

uniform samplerCube u_cubeMap;
uniform vec3        u_SH[9];

uniform vec3        u_eye;
uniform vec3        u_light;

varying vec4        v_position;
varying vec4        v_color;
varying vec3        v_normal;
varying vec2        v_texcoord;

void main(void) {
    vec3 color = vec3(1.0);
    vec3 n = normalize(v_normal);

    // Vector color
    color = v_color.rgb;

    // // IBL, Computing Spherical Harmonics
    vec3 result = max(
           0.282095 * u_SH[0]
        + -0.488603 * u_SH[1] * (n.y)
        +  0.488603 * u_SH[2] * (n.z)
        + -0.488603 * u_SH[3] * (n.x)
        +  1.092548 * u_SH[4] * (n.y * n.x)
        + -1.092548 * u_SH[5] * (n.y * n.z)
        +  0.315392 * u_SH[6] * (3.0 * n.z * n.z - 1.0)
        + -1.092548 * u_SH[7] * (n.z * n.x)
        +  0.546274 * u_SH[8] * (n.x * n.x - n.y * n.y)
        , 0.0);

    // Reinhard tonemapping
    result = result / (result + vec3(1.0));

    // hardcoded compensation
    color *= result;

    // Cheap lighting
    float shade = dot(v_normal, normalize(u_light));
    color *= smoothstep(-1.0, 1.0, shade);

    gl_FragColor = vec4(color, 1.0);
}
