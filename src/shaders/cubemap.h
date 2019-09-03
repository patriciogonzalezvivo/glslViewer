#pragma once

#include <string>

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