#ifdef GL_ES
precision mediump float;
#endif

uniform samplerCube u_cubeMap;
uniform vec3        u_SH[9];

varying vec4 v_position;
varying vec4 v_color;
varying vec3 v_normal;
varying vec2 v_texcoord;

void main(void) {
    vec3 color = vec3(1.0);
    vec3 n = normalize(v_normal);

    // Vector color
    color = v_color.rgb;

    // IBL, Computing Spherical Harmonics
    vec3 SHLightResult[9];
    SHLightResult[0] = 0.282095 * u_SH[0];
    SHLightResult[1] = -0.488603 * n.y * u_SH[1];
    SHLightResult[2] = 0.488603 * n.z * u_SH[2];
    SHLightResult[3] = -0.488603 * n.x * u_SH[3];
    SHLightResult[4] = 1.092548 * n.x * n.y * u_SH[4];
    SHLightResult[5] = -1.092548 * n.y * n.z * u_SH[5];
    SHLightResult[6] = 0.315392 * (3.0 * n.z * n.z - 1.0) * u_SH[6];
    SHLightResult[7] = -1.092548 * n.x * n.z * u_SH[7];
    SHLightResult[8] = 0.546274 * (n.x * n.x - n.y * n.y) * u_SH[8];
    vec3 result = vec3(0.0);
    for (int i = 0; i < 9; ++i)
        result += SHLightResult[i];

    // hardcoded compensation
    color *= result * 0.2;

    // Cheap lighting
    float shade = dot(v_normal, normalize(vec3(0.0, 0.75, 0.75)));
    color *= smoothstep(-1.0, 1.0, shade);

    gl_FragColor = vec4(color, 1.0);
}
