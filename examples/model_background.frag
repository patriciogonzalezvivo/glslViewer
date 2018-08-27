#ifdef GL_ES
precision mediump float;
#endif

uniform vec3 u_light;
uniform vec2 u_resolution;
uniform float u_time;

varying vec4 v_color;
varying vec3 v_normal;

float stroke(float x, float size, float w) {
    float d = step(size, x+w*.5) - step(size, x-w*.5);
    return clamp(d, 0., 1.);
}

vec2 ratio(in vec2 st, in vec2 s) {
    return mix( vec2((st.x*s.x/s.y)-(s.x*.5-s.y*.5)/s.y,st.y),
                vec2(st.x,st.y*(s.y/s.x)-(s.y*.5-s.x*.5)/s.x),
                step(s.x,s.y));
}

float rectSDF(vec2 st, vec2 s) {
    st = st*2.-1.;
    return max( abs(st.x/s.x),
                abs(st.y/s.y) );
}

void main(void) {
   vec4 color = vec4(1.0);
   vec2 st = gl_FragCoord.xy/u_resolution.xy;
   vec2 pixel = 1./u_resolution.xy;

#ifdef BACKGROUND
    st = ratio(st, u_resolution);

    color.rgb *= vec3(0.75, 0.0, 0.0) * step(0.5, fract((st.x - st.y - u_time * 0.1) * 20.));

    float sdf = rectSDF(st, vec2(1.0));
    color.rgb *= step(sdf, 0.7);
    color.rgb += vec3(1.0, 0.0, 0.0) * stroke(sdf, 0.75, 0.01);
#else
    color.rgb = v_color.rgb;
    float shade = dot(v_normal, normalize(vec3(u_light)));
    color.rgb *= smoothstep(-1.0, 1.0, shade);
#endif

    gl_FragColor = color;
}
