#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_scene;
uniform sampler2D   u_sceneDepth;
uniform sampler2D   u_sceneNormal;

uniform vec2        u_resolution;

uniform float       u_cameraFarClip;
uniform float       u_cameraNearClip;
uniform float       u_cameraDistance;

varying vec4        v_position;
varying vec3        v_normal;
varying vec2        v_texcoord;

#define SSAO_DL 2.399963229728653
#define SSAO_RADIUS 0.064
#define SSAO_SAMPLES 10
#define SSAO_NOISE_AMOUNT 0.5
#define SSAO_DEPTH_TEXTURE u_sceneDepth
#define SSAO_RANDOM_FNC(ST) rand(ST)

vec2 rand( const vec2 coord ) { 
    float nx = dot ( coord, vec2( 12.9898, 78.233 ) ); 
    float ny = dot ( coord, vec2( 24.9898, 156.233 ) ); 
    return (clamp( fract ( 43758.5453 * sin( vec2( nx, ny ) ) ), 0.0, 1.0) * 2.0 - 1.0); 
} 

float linearizeDepth(float zoverw) {
	return (2.0 * u_cameraNearClip) / (u_cameraFarClip + u_cameraNearClip - zoverw * (u_cameraFarClip - u_cameraNearClip));	
}

float readDepth( const in vec2 coord ) { 
    float z = 1.0 - linearizeDepth( texture2D( u_sceneDepth, coord ).r ) - u_cameraDistance;
    return z;
} 

float calcAO( float depth, float dw, float dh ) {
    vec2 vv = SSAO_RADIUS * vec2( dw, dh );
    return (step(readDepth( v_texcoord + vv), depth) + step(readDepth( v_texcoord - vv), depth) ) * 0.5;
}

// Thanks to Paul Haeberli ( @GraficaObscura ) for this code
float ssao(vec2 st) {
    vec2 noise = SSAO_RANDOM_FNC( st ) * SSAO_NOISE_AMOUNT; 
    float depth = readDepth( st ); 
    float ao = 1.0;

    if (depth < 0.99) {
        float w = ( 1.0 / u_resolution.x ) / depth + noise.x;
        float h = ( 1.0 / u_resolution.y ) / depth + noise.y;

        float dz = 1.0 / float( SSAO_SAMPLES ); 
        float l = 0.0; 
        float z = 1.0 - dz / 2.0; 

        ao = 0.0; 
        for ( int i = 0; i < SSAO_SAMPLES; i ++ ) { 
            float r = sqrt( 1.0 - z ); 
            float pw = cos( l ) * r; 
            float ph = sin( l ) * r; 
            ao += calcAO( depth, pw * w, ph * h ); 
            z = z - dz; 
            l = l + SSAO_DL; 
        } 
        ao = 1.0 - ao / float( SSAO_SAMPLES ); 
        ao = clamp( 1.98 * ( 1.0 - ao ), 0.0, 1.0 );
    }
    return ao;
}

void main(void) {
    vec3 color = vec3(1.0);
    vec2 uv = v_texcoord;

#ifdef POSTPROCESSING
    color = texture2D(u_scene, uv).rgb;
    color *= ssao(uv);
#endif

    gl_FragColor = vec4(color, 1.0);
}
