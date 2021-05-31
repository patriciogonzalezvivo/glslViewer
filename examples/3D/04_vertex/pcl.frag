

#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_poissonFill;
uniform sampler2D   u_scene;

uniform vec3        u_camera;
uniform vec2        u_resolution;

varying vec4        v_position;
varying vec4        v_color;
varying vec2        v_texcoord;

void main(void) {
    vec4 color = vec4(0.0);
    vec2 st = gl_FragCoord.xy/u_resolution;

// #if defined(POISSON_FILL)
//     color = texture2D(u_scene, st);

// #elif defined(POSTPROCESSING)
//     color = texture2D(u_poissonFill, st);

// #else
    color = v_color;
// #endif

    gl_FragColor = color;
}

