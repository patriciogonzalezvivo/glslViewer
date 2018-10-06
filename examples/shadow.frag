#ifdef GL_ES
precision mediump float;
#endif

uniform vec3        u_light;
uniform sampler2D   u_ligthShadowMap;
varying vec4        v_lightcoord;

varying vec4        v_position;
varying vec4        v_color;
varying vec3        v_normal;
varying vec2        v_texcoord;

#ifdef MODEL_HAS_TANGENTS
varying vec4        v_tangent;
varying mat3        v_tangentToWorld;
#endif

void main(void) {
    vec3 color = vec3(1.0);
    vec2 uv = v_texcoord;

    color = v_color.rgb;

    vec3 shadowCoord = v_lightcoord.xyz / v_lightcoord.w;
    float shadowMap = texture2D(u_ligthShadowMap, shadowCoord.xy).r;

    // From http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/
    float cosTheta = dot(v_normal, normalize(u_light));
    float shade = smoothstep(-1.0, 1.0, cosTheta);

    float bias = 0.005;
    bias *= tan(acos(cosTheta));
    bias = clamp(bias, 0.0, 0.01);

    float shadow = 1.0 - step( shadowMap, shadowCoord.z - bias) * 0.5;
    shadow *= shade;
    color *= 0.5 + shadow * 0.5;

    gl_FragColor = vec4(color, 1.0);
}
