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

std::string wireframe2D_vert = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
uniform mat4 u_modelViewProjectionMatrix;\n\
uniform vec2 u_center;\n\
attribute vec4 a_position;\n\
void main(void) {\n\
    vec4 position = vec4(u_center.x,u_center.y,0.0,0.0) + a_position;\n\
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
    // reflection = reflection / (reflection + vec4(1.0));\n\
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
    gl_FragColor = vec4(1.0, 1.0, 0.0, 1.0);\n\
}";
