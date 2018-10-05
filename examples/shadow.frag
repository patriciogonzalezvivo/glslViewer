#ifdef GL_ES
precision mediump float;
#endif

uniform vec3        u_light;
uniform sampler2D   u_ligthShadowMap;

uniform sampler2D   u_occlusionMap;

varying vec4        v_position;
varying vec3        v_normal;
varying vec2        v_texcoord;
varying vec4        v_lightcoord;

varying vec4        v_color;

#ifdef MODEL_HAS_TANGENTS
varying vec4        v_tangent;
varying mat3        v_tangentToWorld;
#endif

void main(void) {
    vec3 color = vec3(1.0);
    vec2 uv = v_texcoord;

    color = v_color.rgb;

    float ao = texture2D(u_occlusionMap, uv).r;

    vec3 shadowCoord = v_lightcoord.xyz / v_lightcoord.w;
    float shadowMap = texture2D(u_ligthShadowMap, shadowCoord.xy).r;


    // From http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/
    float cosTheta = dot(v_normal, normalize(u_light));
    float shade = smoothstep(-1.0, 1.0, cosTheta);

    float bias = 0.005;
    bias *= tan(acos(cosTheta));
    bias = clamp(bias, 0.0, 0.01);

    float shadow = 1.0;
    if ( shadowMap < shadowCoord.z - bias) {
        shadow = 0.5;
    }
    shadow *= shade;
    
    shadow = pow(shadow, 2.5);
    
    color *= 0.5 + shadow * 0.5;
    color *= ao;    

    gl_FragColor = vec4(color, 1.0);
}
