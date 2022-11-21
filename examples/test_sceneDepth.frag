#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D u_scene;
uniform sampler2D u_sceneDepth;

uniform mat4    u_viewMatrix;

uniform vec3    u_camera;
uniform float   u_cameraFarClip;
uniform float   u_cameraNearClip;
uniform float   u_cameraDistance;

uniform float   u_area;

uniform vec2    u_resolution;

varying vec4    v_position;

#ifdef MODEL_VERTEX_COLOR
varying vec4    v_color;
#endif

#ifdef MODEL_VERTEX_NORMAL
varying vec3    v_normal;
#endif

float linearizeDepth(float depth) {
    depth = 2.0 * depth - 1.0;
    return (2.0 * u_cameraNearClip) / (u_cameraFarClip + u_cameraNearClip - depth * (u_cameraFarClip - u_cameraNearClip));
}

mat3 camera( in vec3 ro) {
    vec3 cw = normalize(-ro);
    vec3 cp = vec3(0.0, 1.0, 0.0);
    vec3 cu = normalize( cross(cw, cp) );
    vec3 cv = normalize( cross(cu, cw) );
    return mat3( cu, cv, cw );
}

vec3 heatmap(float v) {
    vec3 r = v * 2.1 - vec3(1.8, 1.14, 0.3);
    return 1.0 - r * r;
}

void main(void) {
   vec4 color = vec4(1.0);
   vec2 pixel = 1.0/u_resolution;
   vec2 st = gl_FragCoord.xy * pixel;
 
#if defined(POSTPROCESSING)
    vec4 scene = texture2D(u_scene, st); 
    float depth = texture2D(u_sceneDepth, st).r;
    float depthLinear = linearizeDepth(depth);
    depthLinear = u_cameraNearClip + depthLinear * u_cameraFarClip;

    color = scene;
    color.rgb = heatmap( (u_cameraDistance - depthLinear) / u_area * 2. );

    // vec3 ray = camera(u_camera) * normalize(vec3(st*2.0-1.0, 1.8));
    // color = vec4(u_camera + ray * depthLinear, 1.0);

#else
    #ifdef MODEL_VERTEX_COLOR
    color = v_color;
    #endif
#endif

    gl_FragColor = color;
}
