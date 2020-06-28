#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_resolution;
uniform float u_time;

uniform sampler2D u_tex0;
uniform vec2 u_tex0Resolution;
uniform sampler2D u_tex1;
uniform vec2 u_tex1Resolution;
uniform sampler2D u_tex2;
uniform vec2 u_tex2Resolution;
uniform sampler2D u_tex3;
uniform vec2 u_tex3Resolution;

float u_A;
float u_B;

void main (void) {
    vec3 color = vec3(0.0);
    vec2 pixel = 1.0/u_tex0Resolution.xy;
    vec2 uv = gl_FragCoord.xy/u_resolution.xy;
    vec2 st = uv;
    
    float time = u_time;
    float cycle = 1.0;
    float pct = fract(time/cycle);

    vec3 A_FLOW = texture2D(u_tex0, st).rgb;
    vec2 A_DIR = A_FLOW.rg * 2.0 - 1.0;
    A_DIR *= vec2(-1.0, 1.0);
    A_DIR *= pixel * u_A * pct;

    vec3 B_FLOW = texture2D(u_tex2, st).rgb;
    vec2 B_DIR = B_FLOW.rg * 2.0 - 1.0;
    B_DIR *= vec2(-1.0, 1.0);
    B_DIR *= pixel * u_B * (1.0-pct);

    vec3 A_RGB = texture2D(u_tex1, st + A_DIR).rgb;
    vec3 B_RGB = texture2D(u_tex3, st - B_DIR).rgb;

    color += mix(A_RGB, B_RGB, pct);

    gl_FragColor = vec4(color,1.0);
}
