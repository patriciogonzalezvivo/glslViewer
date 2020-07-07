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

uniform float u_A;
uniform float u_B;

#ifndef GAUSSIANBLUR2D_TYPE
#define GAUSSIANBLUR2D_TYPE vec2
#endif

#ifndef GAUSSIANBLUR2D_SAMPLER_FNC
#define GAUSSIANBLUR2D_SAMPLER_FNC(POS_UV) (texture2D(tex, POS_UV).rg * 2.0 - 1.0)
#endif

#ifndef FNC_GAUSSIANBLUR2D
#define FNC_GAUSSIANBLUR2D
GAUSSIANBLUR2D_TYPE gaussianBlur2D(in sampler2D tex, in vec2 st, in vec2 offset, const int kernelSize) {
  GAUSSIANBLUR2D_TYPE accumColor = GAUSSIANBLUR2D_TYPE(0.);
  #ifndef GAUSSIANBLUR2D_KERNELSIZE
  #define GAUSSIANBLUR2D_KERNELSIZE kernelSize
  #endif

  float accumWeight = 0.;
  const float k = .15915494; // 1 / (2*PI)
  float kernelSize2 = float(GAUSSIANBLUR2D_KERNELSIZE) * float(GAUSSIANBLUR2D_KERNELSIZE);

  for (int j = 0; j < GAUSSIANBLUR2D_KERNELSIZE; j++) {
    float y = -.5 * (float(GAUSSIANBLUR2D_KERNELSIZE) - 1.) + float(j);
    for (int i = 0; i < GAUSSIANBLUR2D_KERNELSIZE; i++) {
      float x = -.5 * (float(GAUSSIANBLUR2D_KERNELSIZE) - 1.) + float(i);
      float weight = (k / float(GAUSSIANBLUR2D_KERNELSIZE)) * exp(-(x * x + y * y) / (2. * kernelSize2));
      GAUSSIANBLUR2D_TYPE tex = GAUSSIANBLUR2D_SAMPLER_FNC(st + vec2(x, y) * offset);
      accumColor += weight * tex;
      accumWeight += weight;
    }
  }
  return accumColor / accumWeight;
}
#endif

void main (void) {
    vec3 color = vec3(0.0);
    vec2 pixel = 1.0/u_tex0Resolution.xy;
    vec2 uv = gl_FragCoord.xy/u_resolution.xy;
    vec2 st = uv;
    
    float time = u_time;
    float cycle = 1.0;
    float pct = fract(time/cycle);

    vec2 A_DIR = gaussianBlur2D(u_tex0, st, pixel * 3., 9);
    A_DIR *= vec2(-1.0, 1.0);
    A_DIR *= pixel * u_A * pct;

    vec2 B_DIR = gaussianBlur2D(u_tex2, st, pixel * 3., 9);
    B_DIR *= vec2(-1.0, 1.0);
    B_DIR *= pixel * u_B * (1.0-pct);

    vec3 A_RGB = texture2D(u_tex1, st + A_DIR).rgb;
    vec3 B_RGB = texture2D(u_tex3, st - B_DIR).rgb;
    color = mix(A_RGB, B_RGB, pct);

    // color = vec3((B_DIR * 10.0) * 0.5 + 0.5, 0.0);
    color += step(pct, st.x) * step(st.y, 0.005);

    gl_FragColor = vec4(color,1.0);
}
