#ifdef GL_ES
precision mediump float;
#endif

uniform mat4 u_modelViewProjectionMatrix;
uniform mat4 u_modelMatrix;
uniform mat4 u_viewMatrix;
uniform mat4 u_projectionMatrix;
uniform mat4 u_normalMatrix;

uniform vec4 u_date;
uniform vec2 u_resolution;
uniform vec2 u_mouse;
uniform float u_time;
uniform float u_delta;

varying vec4 v_position;
varying vec4 v_color;
varying vec3 v_normal;
varying vec2 v_texcoord;

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
   vec3 color = vec3(1.0);

#ifdef BACKGROUND
    vec2 st = gl_FragCoord.xy/u_resolution.xy;
    st = ratio(st, u_resolution);

    color *= vec3(0.75, 0.0, 0.0) * step(0.5, fract((st.x - st.y - u_time * 0.1) * 20.));

    float sdf = rectSDF(st, vec2(1.0));
    color *= step(sdf, 0.7);
    color += vec3(1.0, 0.0, 0.0) * stroke(sdf, 0.75, 0.01);

#else
    color = v_color.rgb;
    float shade = dot(v_normal, normalize(vec3(0.0, 0.75, 0.75)));
    color *= smoothstep(-1.0, 1.0, shade);
#endif

    gl_FragColor = vec4(color, 1.0);
}
