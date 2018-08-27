#ifdef GL_ES
precision mediump float;
#endif

uniform vec3 u_light;
uniform vec2 u_resolution;
uniform vec2 u_mouse;
uniform float u_time;

#ifndef BACKGROUND
varying vec4 v_position;
varying vec4 v_color;
varying vec3 v_normal;
#endif
varying vec2 v_texcoord;

void main(void) {
    vec3 color = vec3(0.0);
    vec2 st = v_texcoord;

#ifdef BACKGROUND
    st -= .5;

    float rInv = 1./abs(st.x+st.y);
    st = st * rInv - vec2(rInv,0.0);

    color += smoothstep(.628, .728, abs(sin(st.x * 10.0 -u_time*10.)))*2.;
    color = mix(vec3(0.0), color, 1.-rInv*0.156);
#else
    vec3 normal = v_normal;
    vec3 pos = v_position.xyz;
    
    color = v_color.rgb;
    // color.rgb = v_normal * 0.5 + 0.5;

    float pct = 1.0 - smoothstep(0.2, 0.8, color.r) * 0.5 ;
    pct += smoothstep(-1.0,1.0, -dot(normal, normalize(u_light))) * 0.5;
    color *= step(pct, fract((st.x - st.y) * 2. + u_time));
#endif

    gl_FragColor = vec4(color, 1.0);
}

