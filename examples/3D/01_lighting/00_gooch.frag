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

void main(void) {
    vec3 color = vec3(1.0);

    #ifdef MODEL_VERTEX_COLOR
    color *= v_color.rgb;
    #endif

    vec3 warm = vec3(0.3, 0.2, 0.0) + color * 0.45;
    vec3 cold = vec3(0.0, 0.0, .3) + color * 0.45;

    vec3 n = normalize(v_normal);
    vec3 l = normalize(u_light);
    vec3 v = normalize(u_camera - v_position.xyz);

    float diffuse = (dot(n, l) + 1.0 ) * 0.5;
    color = mix(cold, warm, diffuse);

    // PHONG
    vec3 r = reflect(l, n); // 2.0 * dot(n, l) * n - l;
    float specular = max(0.0, dot(r, -v));

    // // BLINN PHONG
    // vec3 halfVector = normalize(l + v);
    // specular = max(0.0, dot(n, halfVector));

    specular = clamp(100.0 * specular-99.0, 0.0, 1.0);;
    color = mix(color, vec3(1.0, 1.0, 1.0), specular);

    gl_FragColor = vec4(color, 1.0);
}
