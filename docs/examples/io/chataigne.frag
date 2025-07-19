#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_resolution;

uniform float u_x;
uniform float u_y;
uniform vec4 u_color;

varying vec4 v_position;
varying vec4 v_color;
varying vec3 v_normal;
varying vec2 v_texcoord;

#include "../../deps/lygia/draw/stroke.glsl"
#include "../../deps/lygia/sdf/circleSDF.gls

void main (void) {
    vec2 st = gl_FragCoord.xy/u_resolution.xy;
    float aspect = u_resolution.x/u_resolution.y;
    st.x *= aspect;

    vec2 pixel = 2./u_resolution;

    vec3 color = vec3(0.0);
    color += u_color.rgb;
    color.r +=  stroke(st.x,u_x,pixel.x) + 
                stroke(st.y,u_y,pixel.y);

#if defined(DOT)
    color.r += step(circleSDF(st-vec2(u_x,u_y)+.5),.025);
#endif
    
    gl_FragColor = vec4(color,1.0);
}
