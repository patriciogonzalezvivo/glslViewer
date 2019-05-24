#pragma once

#include "glm/glm.hpp"

struct SkyBox {
    SkyBox() {
        groundAlbedo = glm::vec3(0.25);
        azimuth = 0.0f;
        elevation = 0.785398f;
        turbidity = 4.0f;
    }

    glm::vec3   groundAlbedo;
    float       elevation;
    float       azimuth;
    float       turbidity;
    bool        change;
};