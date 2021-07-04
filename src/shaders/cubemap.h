#pragma once

#include <string>

std::string cube_vert = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform mat4    u_modelViewProjectionMatrix;
attribute vec4  a_position;
varying vec4    v_position;

void main(void) {
    v_position = a_position;
    gl_Position = u_modelViewProjectionMatrix * vec4(v_position.xyz, 1.0);
}
)";

std::string cube_frag = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform samplerCube u_cubeMap;

varying vec4        v_position;

void main(void) {
    vec4 reflection = textureCube(u_cubeMap, v_position.xyz);
    reflection.rgb = pow(reflection.rgb, vec3(0.4545454545));
    gl_FragColor = reflection;
}
)";

std::string cube_vert_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform mat4    u_modelViewProjectionMatrix;
in      vec4    a_position;
out     vec4    v_position;

void main(void) {
    v_position = a_position;
    gl_Position = u_modelViewProjectionMatrix * vec4(v_position.xyz, 1.0);
}
)";

std::string cube_frag_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform samplerCube u_cubeMap;

in      vec4        v_position;
out     vec4        fragColor;

void main(void) {
    vec4 reflection = textureCube(u_cubeMap, v_position.xyz);
    reflection.rgb = pow(reflection.rgb, vec3(0.4545454545));
    fragColor = reflection;
}
)";