#pragma once

#include <string>

// 2D
// -----------------------------------------------------

std::string billboard_vert = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
attribute vec4 a_position;\n\
attribute vec2 a_texcoord;\n\
varying vec4 v_position;\n\
varying vec3 v_color;\n\
varying vec3 v_normal;\n\
varying vec2 v_texcoord;\n\
\n\
void main(void) {\n\
    v_position =  a_position;\n\
    v_color = vec3(1.0);\n\
    v_normal = vec3(0.0,0.0,1.0);\n\
    v_texcoord = a_texcoord;\n\
    gl_Position = v_position;\n\
}";

std::string dynamic_billboard_vert = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
uniform mat4 u_modelViewProjectionMatrix;\n\
uniform vec2 u_translate;\n\
uniform vec2 u_scale;\n\
attribute vec4 a_position;\n\
attribute vec2 a_texcoord;\n\
varying vec2 v_texcoord;\n\
\n\
void main(void) {\n\
    vec4 position = a_position;\n\
    position.xy *= u_scale;\n\
    position.xy += u_translate;\n\
    v_texcoord = a_texcoord;\n\
    gl_Position = u_modelViewProjectionMatrix * position;\n\
}";

std::string dynamic_billboard_frag = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
uniform sampler2D u_tex0;\n\
uniform vec4 u_color;\n\
varying vec2 v_texcoord;\n\
\n\
void main(void) { \n\
    vec4 color = u_color;\n\
    color += texture2D(u_tex0, v_texcoord);\n\
    gl_FragColor = color;\n\
}\n";

// wireframe 2D 
std::string wireframe2D_vert = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
uniform mat4 u_modelViewProjectionMatrix;\n\
uniform vec2 u_translate;\n\
uniform vec2 u_scale;\n\
attribute vec4 a_position;\n\
attribute vec2 a_texcoord;\n\
varying vec2 v_texcoord;\n\
\n\
void main(void) {\n\
    vec4 position = a_position;\n\
    position.xy *= u_scale;\n\
    position.xy += u_translate;\n\
    v_texcoord = a_texcoord;\n\
    gl_Position = u_modelViewProjectionMatrix * position;\n\
}\n";

std::string wireframe2D_frag = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
uniform vec4 u_color;\n\
\n\
void main(void) { \n\
    gl_FragColor = u_color;\n\
}\n";

// 3D
// -----------------------------------------------------

std::string cube_vert = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
uniform mat4    u_modelViewProjectionMatrix;\n\
attribute vec4  a_position;\n\
varying vec4    v_position;\n\
\n\
void main(void) {\n\
    v_position = a_position;\n\
    gl_Position = u_modelViewProjectionMatrix * vec4(v_position.xyz, 1.0);\n\
}";

std::string cube_frag = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
uniform samplerCube u_cubeMap;\n\
\n\
varying vec4    v_position;\n\
\n\
void main(void) {\n\
    vec4 reflection = textureCube(u_cubeMap, v_position.xyz);\n\
    //reflection = reflection / (reflection + vec4(1.0));\n\
    //reflection.rgb = pow(reflection.rgb, vec3(0.4545454545));\n\
    gl_FragColor = reflection;\n\
}";

std::string wireframe3D_vert = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
uniform mat4    u_modelViewProjectionMatrix;\n\
attribute vec4  a_position;\n\
\n\
void main(void) {\n\
    gl_Position = u_modelViewProjectionMatrix * a_position;\n\
}";

std::string wireframe3D_frag = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
uniform vec4 u_color;\n\
\n\
void main(void) {\n\
    gl_FragColor = u_color;\n\
}";

std::string light_vert = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
uniform mat4    u_modelViewProjectionMatrix;\n\
uniform mat4    u_viewMatrix;\n\
uniform vec3    u_translate;\n\
uniform vec2    u_scale;\n\
attribute vec4  a_position;\n\
attribute vec2  a_texcoord;\n\
varying vec2    v_texcoord;\n\
\n\
void main(void) {\n\
    vec4 pos = a_position;\n\
    v_texcoord = a_texcoord;\n\
    \n\
    vec3 cam_right = vec3(u_viewMatrix[0][0], u_viewMatrix[1][0], u_viewMatrix[2][0]);\n\
    vec3 cam_up = vec3(u_viewMatrix[0][1], u_viewMatrix[1][1], u_viewMatrix[2][1]);\n\
    \n\
    pos.xyz -= cam_right * (v_texcoord.x - 0.5) * u_scale.x;\n\
    pos.xyz -= cam_up * (v_texcoord.y - 0.5) * u_scale.y;\n\
    pos.xyz += u_translate;\n\
    \n\
    gl_Position = u_modelViewProjectionMatrix * pos;\n\
}";

std::string light_frag = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
uniform vec4    u_color;\n\
varying vec2    v_texcoord;\n\
\n\
#define AA_EDGE .007\n\
float stroke(float x, float size, float w, float edge) {\n\
    float d = smoothstep(size - edge, size + edge, x + w * .5) - smoothstep(size - edge, size + edge, x - w * .5);\n\
    return clamp(d, 0., 1.);\n\
}\n\
\n\
float stroke(float x, float size, float w) {\n\
	return stroke(x, size, w, AA_EDGE);\n\
}\n\
\n\
float fill(float x, float size, float edge) {\n\
    return 1. - smoothstep(size - edge, size + edge, x);\n\
}\n\
\n\
float fill(float x, float size) {\n\
    return fill(x, size, AA_EDGE);\n\
}\n\
\n\
float circleSDF(in vec2 st, in vec2 center) {\n\
  return length(st - center) * 2.;\n\
}\n\
\n\
float circleSDF(in vec2 st) {\n\
  return circleSDF(st, vec2(.5));\n\
}\n\
\n\
void main(){\n\
    vec3 color = u_color.rgb;\n\
    float alpha = 0.0;\n\
    vec2 st = v_texcoord;\n\
    \n\
    float sdf = circleSDF(st);\n\
    alpha += stroke((st.y+st.x)*0.5, .5, .03);\n\
    alpha += stroke((st.x-st.y+0.5), .5, .06);\n\
    alpha *= fill(sdf, .9);\n\
    alpha += stroke(st.x, .5, .05);\n\
    alpha += stroke(st.y, .5, .05);\n\
    alpha *= fill(sdf, .95);\n\
    alpha *= smoothstep(.57, .58, sdf);\n\
    alpha += stroke(sdf, .4, .15);\n\
    \n\
    gl_FragColor = vec4(color, alpha);\n\
}";
