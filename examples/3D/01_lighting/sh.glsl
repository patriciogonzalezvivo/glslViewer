#include "tonemap.glsl"

#ifndef FNC_SH
#define FNC_SH
uniform vec3        u_SH[9];

vec3 sh(vec3 n) {
    return max(
           0.282095 * u_SH[0]
        + -0.488603 * u_SH[1] * (n.y)
        +  0.488603 * u_SH[2] * (n.z)
        + -0.488603 * u_SH[3] * (n.x)
        +  1.092548 * u_SH[4] * (n.y * n.x)
        + -1.092548 * u_SH[5] * (n.y * n.z)
        +  0.315392 * u_SH[6] * (3.0 * n.z * n.z - 1.0)
        + -1.092548 * u_SH[7] * (n.z * n.x)
        +  0.546274 * u_SH[8] * (n.x * n.x - n.y * n.y)
        , 0.0);
}
#endif