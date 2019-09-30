#pragma once

#include "node.h"
#include "../gl/fbo.h"

enum LightType {
    LIGHT_DIRECTIONAL, LIGHT_POINT, LIGHT_SPOT
};

class Light : public Node {
public:
    Light();
    virtual ~Light();

    const LightType&    getType() const { return m_type; }
    glm::mat4           getMVPMatrix( const glm::mat4 &_model, float _area );
    glm::mat4           getBiasMVPMatrix();

    const Fbo*          getShadowMap() const { return &m_shadowMap; }

    void                bindShadowMap();
    void                unbindShadowMap();

    glm::vec3           color;


protected:
    virtual void        onPositionChanged() { bChange = true; };
    virtual void        onOrientationChanged() { bChange = true; };
    virtual void        onScaleChanged() { bChange = true; };

private:
    Fbo                 m_shadowMap;
    glm::mat4           m_mvp_biased;
    glm::mat4           m_mvp;
    
    LightType           m_type;
};
