

#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_scene;

uniform sampler2D   u_buffer0;
uniform sampler2D   u_buffer1;

uniform vec2        u_resolution;

varying vec4        v_color;
varying vec2        v_texcoord;

void main(void) {
    vec4 color = vec4(0.0);
    vec2 st = v_texcoord;

#if defined(BUFFER_0)
    color = texture2D(u_buffer1, st) * 0.98;
    color += texture2D(u_scene, st);

#elif defined(BUFFER_1)
    color = texture2D(u_buffer0, st);

#elif defined(POSTPROCESSING)
    vec2 pixel = 1.0/u_resolution;
    float sdf = length(st-0.5);

    st = ((st-0.5) * 0.95) + 0.5;
    color.r = texture2D(u_buffer0, st + pixel * 10.0 * sdf).r;
    color.g = texture2D(u_buffer0, st).g;
    color.b = texture2D(u_buffer0, st - pixel * 10.0 * sdf).b;
    color.a = 1.0;

#else
    color = v_color;
#endif

    gl_FragColor = color;
}

