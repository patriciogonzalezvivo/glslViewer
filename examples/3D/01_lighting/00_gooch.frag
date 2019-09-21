#ifdef GL_ES
precision mediump float;
#endif

uniform vec3        u_camera;
uniform vec3        u_light;
uniform vec2        u_resolution;

varying vec4        v_position;

#ifdef MODEL_VERTEX_COLOR
varying vec4        v_color;
#endif

#ifdef MODEL_VERTEX_NORMAL
varying vec3        v_normal;
#endif

#ifdef MODEL_VERTEX_TANGENT
varying vec4        v_tangent;
varying mat3        v_tangentToWorld;
#endif

#ifdef MODEL_VERTEX_TEXCOORD
varying vec2        v_texcoord;
#endif

// https://github.com/glslify/glsl-specular-phong
float phongSpecular(vec3 L, vec3 N, vec3 V, float shininess) {
    vec3 R = reflect(L, N); // 2.0 * dot(N, L) * N - L;
    return pow(max(0.0, dot(R, -V)), shininess);
}

// https://github.com/glslify/glsl-specular-blinn-phong
float blinnPhongSpecular(vec3 L, vec3 N, vec3 V, float shininess) {
    // halfVector
    vec3 H = normalize(L + V);
    return pow(max(0.0, dot(N, H)), shininess);
}

void main(void) {
    vec3 color = vec3(1.0);

    #ifdef MODEL_VERTEX_COLOR
    color *= v_color.rgb;
    #endif

    vec3 warm = vec3(0.3, 0.2, 0.0) + color * 0.45;
    vec3 cold = vec3(0.0, 0.0, .3) + color * 0.45;

    vec3 l = normalize(u_light - v_position.xyz);
    vec3 n = normalize(v_normal);
    vec3 v = normalize(u_camera - v_position.xyz);
    float shininess = 20.0;

    // Lambert Diffuse
    float diffuse = max(0.0, dot(n, l));

    // Gooch light model
    // Warmer towards the light, colder in the opesite direction
    color = mix(cold, warm, diffuse);

    // Phong Specular
    float specular = phongSpecular(l, n, v, shininess);
    // specular = blinnPhongSpecular(l, n, v, shininess);

    color = mix(color, vec3(1.0, 1.0, 1.0), specular);

    gl_FragColor = vec4(color, 1.0);
}
