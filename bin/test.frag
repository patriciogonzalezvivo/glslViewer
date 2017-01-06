#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_mouse;
uniform vec2 u_resolution;
uniform float u_time;

uniform sampler2D u_tex0;
uniform vec2 u_tex0Resolution;

varying vec4 v_position;
varying vec4 v_color;
varying vec3 v_normal;
varying vec2 v_texcoord;

#define HALF_PI 1.5707963267948966192313216916398
#define PI 3.1415926535897932384626433832795
#define TWO_PI 6.2831853071795864769252867665590
#define TAU 6.2831853071795864769252867665590
#define PHI 1.618033988749894848204586834
#define EPSILON 0.0000001

float fill (float x, float size) {
    return 1.-step(size, x);
}

float stroke (float x, float size, float w) {
    float d = step(size, x+w*.5) - step(size, x-w*.5);
    return clamp(d, 0., 1.);
}

float rectSDF (vec2 st, vec2 size) {
    st -= .5;
    return max( abs(st.x/size.x),
                abs(st.y/size.y));
}

float crossSDF (vec2 st, float s) {
    vec2 size = vec2(.25, s) * .5;
    return min( rectSDF(st.xy,size.xy),
                rectSDF(st.xy,size.yx));
}

float circleSDF (vec2 st) {
    return length(st-.5)*2.;
}

float spiralSDF (vec2 st, float turns) {
    st -= .5;
    float r = dot(st,st);
    float a = atan(st.y,st.x);
    return abs(sin(fract(log(r)*turns+a*0.159)));
}

float shapeSDF (vec2 st, int N) {
    st = st*2.-1.;
    float a = atan(st.x,st.y)+PI;
    float r = TWO_PI/float(N);
    return cos(floor(.5+a/r)*r-a)*length(st);
}

float starSDF (vec2 st, int N, float size) {
    st = st*4.-2.;
    float a = atan(st.y, st.x) / TWO_PI;
    float seg = a * float(N);
    a = (floor(seg) + 0.5) / float(N);
    a += mix(size,-size,step(.5,fract(seg)));
    a *= TWO_PI;
    return abs(dot(vec2(cos(a),sin(a)), st));
}

void main () {
	vec3 color = vec3(0.);
    vec2 st = gl_FragCoord.xy/u_resolution;
    st = (st-.5)*1.1+.5;
    if (u_resolution.y > u_resolution.x ) {
        st.y *= u_resolution.y/u_resolution.x;
        st.y -= (u_resolution.y*.5-u_resolution.x*.5)/u_resolution.x;
    } else {
        st.x *= u_resolution.x/u_resolution.y;
        st.x -= (u_resolution.x*.5-u_resolution.y*.5)/u_resolution.y;
    }
    //START
    // color.rgb += fill(rectSDF(st,vec2(.5,.1)),.5);
    color.rgb += fill(crossSDF(st,1.9),.25);
    // color.rgb += stroke(circleSDF(st),.5,.1);
    // color.rgb += stroke(spiralSDF(st,.1),.5,.15);
    // color.rgb += stroke(shapeSDF(st, 3),.5,.15);
    color.rgb += stroke(starSDF(st, 6,.1),.5,.15);
    //END
    gl_FragColor = vec4(color,1.);
}
