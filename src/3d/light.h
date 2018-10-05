#pragma once

#include "node.h"

enum LightType {
    LIGHT_DIRECTIONAL, LIGHT_POINT, LIGHT_SPOT
};

class Light : public Node {
public:
    Light();
    virtual ~Light();

    const LightType&    getType() const { return m_type; }
    glm::mat4           getMVPMatrix( glm::mat4 _model );

    glm::vec3   color;

    bool        bChange;

protected:
    virtual void        onPositionChanged() { bChange = true; };
    virtual void        onOrientationChanged() { bChange = true; };
    virtual void        onScaleChanged() { bChange = true; };

private:
    LightType m_type;
};
