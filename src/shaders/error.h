#pragma once

#include <string>

std::string error_frag = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
void main(void) {\n\
    vec3 color = vec3(1.0, 0.0, 1.0);\n\
    gl_FragColor = vec4(color, 1.0);\n\
}\n\
";