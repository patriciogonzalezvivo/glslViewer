#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_resolution;

// Uniforms set by Orca via OSC
// Orca can only send integer data (int, ivec2, ivec3, ivec4)
// Make sure that Orca is sending data to the port glslViewer is listening to
uniform int a;
uniform int b;
uniform int c;
uniform ivec2 e;

vec3 circle(vec2 st, float size, vec3 c) {
    float l = 1. - length(st);
    float edge = size;
    float l1 = step(edge, l);
    float l2 = smoothstep(edge, edge + .341, l);
    l = l1 - l2;
    return c = vec3(pow(l, 5.) * c);
}

void main() {
    vec2 st = gl_FragCoord.xy/u_resolution;
    vec3 color = vec3(0.);    
    vec3 c_bg = vec3(.133, .161, .224);
    vec3 c_fg = vec3(.306, .514, .549);
    color = c_bg;    
    st += vec2(mix(-.1, .1, float(e.x) / 10.), mix(-.1, .1, float(e.y) / 10.));
    color += circle(st - .5, float(a) / 10., c_fg * 1.) * .216;
    color += circle(st - .3, float(b) / 10., c_fg * .5) * .216;
    color += circle(st - .7, float(c) / 10., c_fg * 1.5) * .216;
	  gl_FragColor = vec4(color, 1.);
}
