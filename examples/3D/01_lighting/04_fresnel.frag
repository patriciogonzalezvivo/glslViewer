#ifdef GL_ES
precision mediump float;
#endif

// By Karim Naaji
// https://github.com/karimnaaji/hdreffects/blob/master/build/shaders/fresnel.frag


uniform samplerCube u_cubeMap;

varying vec4 v_position;
varying vec4 v_color;
varying vec3 v_normal;
varying vec2 v_texcoord;

varying vec3 v_reflect;
varying vec3 v_refractR;
varying vec3 v_refractG;
varying vec3 v_refractB;
varying float v_ratio;

void main(void) {
   vec3 color = vec3(1.0);

    vec3 refractColor, reflectColor;
    refractColor.r = vec3(textureCube(u_cubeMap, v_refractR)).r;
    refractColor.g = vec3(textureCube(u_cubeMap, v_refractG)).g;
    refractColor.b = vec3(textureCube(u_cubeMap, v_refractB)).b;
    reflectColor = vec3(textureCube(u_cubeMap, v_reflect));

    color = mix(refractColor, reflectColor, v_ratio);

    gl_FragColor = vec4(color, 1.0);
}
