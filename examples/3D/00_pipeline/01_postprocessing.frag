#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D u_scene;
uniform sampler2D u_sceneDepth;

uniform vec3    u_camera;
uniform float   u_cameraFarClip;
uniform float   u_cameraNearClip;
uniform float   u_cameraDistance;

uniform vec3    u_light;
uniform vec2    u_resolution;
uniform float   u_time;

varying vec4    v_position;

#ifdef MODEL_VERTEX_COLOR
varying vec4    v_color;
#endif

#ifdef MODEL_VERTEX_NORMAL
varying vec3    v_normal;
#endif

#include "../../2D/02_pixelspiritdeck/lib/ratio.glsl"
#include "../../2D/02_pixelspiritdeck/lib/stroke.glsl"

float linearizeDepth(float depth) {
    depth = 2.0 * depth - 1.0;
    return (2.0 * u_cameraNearClip) / (u_cameraFarClip + u_cameraNearClip - depth * (u_cameraFarClip - u_cameraNearClip));
}

void main(void) {
   vec3 color = vec3(1.0);
   vec2 st = gl_FragCoord.xy/u_resolution.xy;
   
#ifdef BACKGROUND
    st = ratio(st, u_resolution);
    color *= vec3(0.5) * step(0.5, fract((st.x - st.y - u_time * 0.05) * 20.));

#elif defined(POSTPROCESSING)
    vec3 scene = texture2D(u_scene, st).rgb;
    float depth = texture2D(u_sceneDepth, st).r;
    depth = linearizeDepth(depth) * u_cameraFarClip;
    depth = clamp( (depth - u_cameraDistance + sin(u_time * 0.5) * 5.0), 0.0 , 1.0);

    // Box blur
    vec2 pixel = 1./u_resolution.xy;
    pixel *= 4.;
    vec3 boxblur = texture2D(u_scene, st + vec2(pixel.x, 0.0)).rgb;
    boxblur += texture2D(u_scene, st + vec2(0.0, pixel.y)).rgb;
    boxblur += texture2D(u_scene, st + vec2(-pixel.x, 0.0)).rgb;
    boxblur += texture2D(u_scene, st + vec2(0.0, -pixel.y)).rgb;
    boxblur *= 0.125;

    color = mix(scene, boxblur, depth);
    // color = mix(color, vec3(depth), step(0.5, st.x));
#else
    #ifdef MODEL_VERTEX_COLOR
    color *= v_color.rgb;
    #endif

    vec3 n = normalize(v_normal);
    vec3 l = normalize(u_light);
    float diffuse = (dot(n, l) + 1.0 ) * 0.5;

    color *= diffuse;
    color *= step(fract((st.x + st.y) * 70.), diffuse);
#endif

    gl_FragColor = vec4(color, 1.0);
}
