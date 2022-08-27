#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_scene;
uniform sampler2D   u_sceneNormal;
uniform sampler2D   u_scenePosition;

uniform mat4        u_viewMatrix;
uniform mat4        u_projectionMatrix;
uniform mat3        u_normalMatrix;

#define NUM_SAMPLES 8
#define NUM_NOISE   4

uniform vec3        u_ssaoSamples[NUM_SAMPLES];
uniform vec3        u_ssaoNoise[NUM_NOISE];

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

#ifdef MODEL_VERTEX_TEXCOORD
varying vec2        v_texcoord;
#endif

void main(void) {
    vec4 color = vec4(1.0);
    vec2 pixel = 1.0/u_resolution;
    vec2 st = gl_FragCoord.xy * pixel;

#if defined(POSTPROCESSING)
    color = texture2D(u_scene, st);

    float radius    = 0.2;
    float bias      = 0.005;
    float magnitude = 1.1;
    float contrast  = 1.1;

    vec4 position = texture2D(u_scenePosition, st);
    vec3 normal = ( texture2D(u_sceneNormal, st).rgb);

    float noiseS = sqrt(float(NUM_NOISE));
    int  noiseX = int( mod(gl_FragCoord.x - 0.5, noiseS) );
    int  noiseY = int( mod(gl_FragCoord.y - 0.5, noiseS) );
    vec3 random = u_ssaoNoise[noiseX + noiseY * int(noiseS)];

    vec3 tangent  = normalize(random - normal * dot(random, normal));
    vec3 binormal = cross(normal, tangent);
    mat3 tbn      = mat3(tangent, binormal, normal);

    float occlusion = 0.0;
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        vec3 samplePosition = tbn * u_ssaoSamples[i];
        samplePosition = position.xyz + samplePosition * radius;

        vec4 offsetUV = vec4(samplePosition, 1.0);
        offsetUV = u_projectionMatrix * offsetUV;
        offsetUV.xy /= offsetUV.w;
        offsetUV.xy = offsetUV.xy * 0.5 + 0.5;

        float sampleDepth = texture2D(u_scenePosition, offsetUV.xy).z;
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(position.z - sampleDepth));
        occlusion += (sampleDepth >= samplePosition.z + bias ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion /= float(NUM_SAMPLES);
    occlusion  = pow(occlusion, magnitude);
    occlusion  = contrast * (occlusion - 0.5) + 0.5;
    color.rgb *= 1.0-occlusion;


#else

    #ifdef FLOOR
    vec2 uv = v_texcoord;
    uv = floor(fract(uv * 8.0) * 2.0);
    color.rgb = vec3(0.5) + (min(1.0, uv.x + uv.y) - (uv.x * uv.y)) * 0.5;
    #endif

    #ifdef MODEL_VERTEX_NORMAL
    vec3 n = normalize(v_normal);
    vec3 l = normalize(u_light);
    vec3 v = normalize(u_camera - v_position.xyz);

    color.rgb *= (dot(n, l) + 1.0 ) * 0.5;
    #endif

#endif

    gl_FragColor = color;
}

