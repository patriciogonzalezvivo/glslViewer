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

#include "lygia/space/linearizeDepth.glsl"
#include "lygia/color/palette/heatmap.glsl"

void main(void) {
   vec4 color = vec4(1.0);
   vec2 pixel = 1.0/u_resolution;
   vec2 st = gl_FragCoord.xy * pixel;
 
#if defined(POSTPROCESSING)
    vec4 scene = texture2D(u_scene, st); 
    float depth = texture2D(u_sceneDepth, st).r;
    float depthLinear = linearizeDepth(depth, u_cameraNearClip, u_cameraFarClip);

    color = scene;
    color.rgb = heatmap( (u_cameraDistance - depthLinear) / u_area * 2. );
#else
    #ifdef MODEL_VERTEX_COLOR
    color = v_color;
    #endif
#endif

    gl_FragColor = color;
}
