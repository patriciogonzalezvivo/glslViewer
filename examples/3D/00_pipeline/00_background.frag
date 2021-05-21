#ifdef GL_ES
precision mediump float;
#endif

uniform vec3    u_light;
uniform vec2    u_resolution;

#ifdef HOLOPLAY
uniform vec4    u_holoPlayViewport;
#define RESOLUTION u_holoPlayViewport.zw
#else
#define RESOLUTION u_resolution
#endif

uniform float   u_time;

#ifdef MODEL_VERTEX_COLOR
varying vec4    v_color;
#endif

#ifdef MODEL_VERTEX_NORMAL
varying vec3    v_normal;
#endif

#include "../../2D/02_pixelspiritdeck/lib/ratio.glsl"
#include "../../2D/02_pixelspiritdeck/lib/stroke.glsl"

void main(void) {
   vec3 color = vec3(1.0);

   vec2 st = gl_FragCoord.xy/RESOLUTION;

#ifdef BACKGROUND
    st = ratio(st, u_resolution);
    color *= vec3(0.5) * step(0.5, fract((st.x - st.y - u_time * 0.05) * 20.));
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
