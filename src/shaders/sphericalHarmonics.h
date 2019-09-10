#pragma once

#include <string>

const std::string sphericalHarmonics = "\n\
\n\
uniform samplerCube u_cubeMap;\n\
uniform vec3        u_SH[9];\n\
\n\
vec3 sphericalHarmonics(const vec3 n) {\n\
    return max(\n\
           0.282095 * u_SH[0]\n\
        + -0.488603 * u_SH[1] * (n.y)\n\
        +  0.488603 * u_SH[2] * (n.z)\n\
        + -0.488603 * u_SH[3] * (n.x)\n\
        , 0.0);\n\
}\n\
\n\
vec3 tonemap(const vec3 x) { return x / (x + 0.155) * 1.019; }\n\
\n";