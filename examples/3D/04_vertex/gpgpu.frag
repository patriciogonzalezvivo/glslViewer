

#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_scene;

// position
uniform sampler2D   u_buffer0;
uniform sampler2D   u_buffer1;

// velocity
uniform sampler2D   u_buffer2;
uniform sampler2D   u_buffer3;

uniform vec2        u_resolution;
uniform float       u_time;
uniform int         u_frame;      

varying vec4        v_color;
varying vec2        v_texcoord;

vec3 random3(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * vec3(0.3,0.6,0.2));
    p3 += dot(p3, p3.yxz+19.19);
    return fract((p3.xxy+p3.yzz)*p3.zyx);
}

vec3 random3(in vec3 p) {
    p = vec3( dot(p, vec3(127.1, 311.7, 74.7)),
            dot(p, vec3(269.5, 183.3, 246.1)),
            dot(p, vec3(113.5, 271.9, 124.6)));
    return -1. + 2. * fract(sin(p) * 43758.5453123);
}

// returns 3D value noise (in .x)  and its derivatives (in .yzw)
// https://www.shadertoy.com/view/4dffRH
vec4 noised (in vec3 pos) {
  // grid
  vec3 p = floor(pos);
  vec3 w = fract(pos);

#ifdef NOISED_QUINTIC_INTERPOLATION
  // quintic interpolant
  vec3 u = w * w * w * ( w * (w * 6. - 15.) + 10. );
  vec3 du = 30.0 * w * w * ( w * (w - 2.) + 1.);
#else
  // cubic interpolant
  vec3 u = w * w * (3. - 2. * w);
  vec3 du = 6. * w * (1. - w);
#endif

  // gradients
  vec3 ga = random3(p + vec3(0., 0., 0.));
  vec3 gb = random3(p + vec3(1., 0., 0.));
  vec3 gc = random3(p + vec3(0., 1., 0.));
  vec3 gd = random3(p + vec3(1., 1., 0.));
  vec3 ge = random3(p + vec3(0., 0., 1.));
  vec3 gf = random3(p + vec3(1., 0., 1.));
  vec3 gg = random3(p + vec3(0., 1., 1.));
  vec3 gh = random3(p + vec3(1., 1., 1.));

  // projections
  float va = dot(ga, w - vec3(0., 0., 0.));
  float vb = dot(gb, w - vec3(1., 0., 0.));
  float vc = dot(gc, w - vec3(0., 1., 0.));
  float vd = dot(gd, w - vec3(1., 1., 0.));
  float ve = dot(ge, w - vec3(0., 0., 1.));
  float vf = dot(gf, w - vec3(1., 0., 1.));
  float vg = dot(gg, w - vec3(0., 1., 1.));
  float vh = dot(gh, w - vec3(1., 1., 1.));

  // interpolations
  return vec4( va + u.x*(vb-va) + u.y*(vc-va) + u.z*(ve-va) + u.x*u.y*(va-vb-vc+vd) + u.y*u.z*(va-vc-ve+vg) + u.z*u.x*(va-vb-ve+vf) + (-va+vb+vc-vd+ve-vf-vg+vh)*u.x*u.y*u.z,    // value
               ga + u.x*(gb-ga) + u.y*(gc-ga) + u.z*(ge-ga) + u.x*u.y*(ga-gb-gc+gd) + u.y*u.z*(ga-gc-ge+gg) + u.z*u.x*(ga-gb-ge+gf) + (-ga+gb+gc-gd+ge-gf-gg+gh)*u.x*u.y*u.z +   // derivatives
               du * (vec3(vb,vc,ve) - va + u.yzx*vec3(va-vb-vc+vd,va-vc-ve+vg,va-vb-ve+vf) + u.zxy*vec3(va-vb-ve+vf,va-vb-vc+vd,va-vc-ve+vg) + u.yzx*u.zxy*(-va+vb+vc-vd+ve-vf-vg+vh) ));
}

void main(void) {
    vec3 color = vec3(0.0);
    vec2 st = v_texcoord;
    float alpha = 1.0;

#if defined(BUFFER_0)
    vec3 pos = texture2D(u_buffer1, st).xyz;
    vec3 vel = texture2D(u_buffer3, st).xyz * 2.0 - 1.0;

    pos += vel * 0.001;

    color = u_frame < 1 ? random3(st) : fract(pos);

#elif defined(BUFFER_1)
    color = texture2D(u_buffer0, st).rgb;

#elif defined(BUFFER_2)
    vec3 pos = texture2D(u_buffer1, st).xyz - 0.5;
    vec3 vel = texture2D(u_buffer3, st).xyz * 2.0 - 1.0;

    vec3 rnd = noised(pos * 5.).yzw;
    vel += rnd * 0.05;

    vel *= 0.991;
    color = u_frame < 1 ? rnd : clamp(vel, -1.0, 1.0) * 0.5 + 0.5;

#elif defined(BUFFER_3)
    color = texture2D(u_buffer2, st).rgb;

#else
    color = v_color.rgb;
    alpha = v_color.a;
#endif

    gl_FragColor = vec4(color, alpha);
}

