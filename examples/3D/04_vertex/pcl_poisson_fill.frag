#ifdef GL_ES
precision mediump float;
#endif


// Copyright Patricio Gonzalez Vivo, 2021 - http://patriciogonzalezvivo.com/
// I am the sole copyright owner of this Work.
//
// You cannot host, display, distribute or share this Work in any form,
// including physical and digital. You cannot use this Work in any
// commercial or non-commercial product, website or project. You cannot
// sell this Work and you cannot mint an NFTs of it.
// I share this Work for educational purposes, and you can link to it,
// through an URL, proper attribution and unmodified screenshot, as part
// of your educational material. If these conditions are too restrictive
// please contact me and we'll definitely work it out.


uniform sampler2D   u_tex0Depth;

uniform sampler2D   u_scene;
uniform sampler2D   u_sceneDepth;

uniform sampler2D   u_buffer0;
uniform sampler2D   u_buffer1;
uniform sampler2D   u_buffer2;
uniform sampler2D   u_buffer3;

uniform sampler2D   u_pyramid0;
uniform sampler2D   u_pyramidTex0;
uniform sampler2D   u_pyramidTex1;
uniform bool        u_pyramidUpscaling;

uniform vec2        u_resolution;
uniform float       u_cameraFarClip;
uniform float       u_cameraNearClip;
uniform float       u_cameraDistance;
uniform float       u_time;

varying vec4        v_color;
varying vec2        v_texcoord;

#define saturate(x) clamp(x, 0.0, 1.0)
float   linearizeDepth(float depth);
vec4    gaussianBlur2D(in sampler2D tex, in vec2 st, in vec2 offset, const int kernelSize);
vec3    normalMap(sampler2D tex, vec2 st, vec2 pixel);
float   rgb2luma(in vec3 color);

void main(void) {
    vec4 color = vec4(0.0);
    vec2 st = v_texcoord;
    vec2 pixel = 1./u_resolution;

    float depth = texture2D(u_sceneDepth, v_texcoord).r;
    depth = linearizeDepth(depth) * u_cameraFarClip;
    depth = clamp( (depth - u_cameraDistance), 0.0 , 1.0);

#if defined(BUFFER_0)
    color.rgb = normalMap(u_tex0Depth, st, pixel * 5.) * 0.5 + 0.5;
    color.a = 1.0;

#elif defined(BUFFER_1)
    color += 1.0-depth;
    color.a = 1.0;

#elif defined(BUFFER_2)
    color = step(0.1, gaussianBlur2D(u_buffer1, st, pixel * 6., 16));
    color.a = 1.0;

#elif defined(BUFFER_3)
    color = smoothstep(0.0, 0.3, gaussianBlur2D(u_buffer2, st, pixel * 4., 16));
    color.a = 1.0;

#elif defined(CONVOLUTION_PYRAMID_0)
    color = texture2D(u_scene, st);
    color *= step(0.5, texture2D(u_buffer3, st).r);

#elif defined(POSTPROCESSING)
    color = texture2D(u_pyramid0, st) * 1.;
    vec4 scene = texture2D(u_scene, st);
    float luma = rgb2luma(scene.rgb);
    color = mix(color * 0.9, scene, smoothstep(0.5,.9,luma));
    color.a = 0.1 + texture2D(u_buffer3, st).r * 0.9;

#else
    color = v_color;

    vec3 normal = texture2D(u_buffer0, v_texcoord).rgb * 2.0 - 1.0;
    color *= 1.0-pow(1.0-abs(dot(normal, vec3(0.0,0.0,1.0))), 16.);

#endif

    gl_FragColor = color;
}


// Usefull functions

float linearizeDepth(float depth) {
    depth = 2.0 * depth - 1.0;
    return (2.0 * u_cameraNearClip) / (u_cameraFarClip + u_cameraNearClip - depth * (u_cameraFarClip - u_cameraNearClip));
}

vec4 gaussianBlur2D(in sampler2D tex, in vec2 st, in vec2 offset, const int kernelSize) {
    vec4 accumColor = vec4(0.);

    float accumWeight = 0.;
    const float k = .15915494; // 1 / (2*PI)
    float kernelSize2 = float(kernelSize) * float(kernelSize);

    for (int j = 0; j < kernelSize; j++) {
        float y = -.5 * (float(kernelSize) - 1.) + float(j);
        for (int i = 0; i < kernelSize; i++) {
            float x = -.5 * (float(kernelSize) - 1.) + float(i);
            float weight = (k / float(kernelSize)) * exp(-(x * x + y * y) / (2.0 * kernelSize2));
            vec4 tex = texture2D(tex, saturate(st + vec2(x, y) * offset));
            accumColor += weight * tex;
            accumWeight += weight;
        }
    }
    return accumColor / accumWeight;
}

vec3 normalMap(sampler2D tex, vec2 st, vec2 pixel) {
    float center      = texture2D(tex, st).r;
    float topLeft     = texture2D(tex, st - pixel).r;
    float left        = texture2D(tex, st - vec2(pixel.x, .0)).r;
    float bottomLeft  = texture2D(tex, st + vec2(-pixel.x, pixel.y)).r;
    float top         = texture2D(tex, st - vec2(.0, pixel.y)).r;
    float bottom      = texture2D(tex, st + vec2(.0, pixel.y)).r;
    float topRight    = texture2D(tex, st + vec2(pixel.x, -pixel.y)).r;
    float right       = texture2D(tex, st + vec2(pixel.x, .0)).r;
    float bottomRight = texture2D(tex, st + pixel).r;
    
    float dX = topRight + 2. * right + bottomRight - topLeft - 2. * left - bottomLeft;
    float dY = bottomLeft + 2. * bottom + bottomRight - topLeft - 2. * top - topRight;

    return normalize(vec3(dX, dY, 0.01) );
}

float rgb2luma(in vec3 color) {
    return dot(color, vec3(0.299, 0.587, 0.114));
}
