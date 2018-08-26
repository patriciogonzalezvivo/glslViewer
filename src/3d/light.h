#pragma once

#include "node.h"

enum LightType {
    LIGHT_DIRECTIONAL, LIGHT_POINT, LIGHT_SPOT
};

class Light : public Node {
public:
    Light(): color(1.0), exposure(1.0), ev100(1.0), m_type(LIGHT_POINT){};
    virtual ~Light() {}

    const LightType&   getType() const { return m_type; }
    glm::vec3   color;
    float   exposure; 
    float   ev100;

private:
    LightType m_type;
};
