#include "light.h"

Light::Light(): color(1.0), m_mvp_biased(1.0), m_mvp(1.0), m_type(LIGHT_DIRECTIONAL) {

}

Light::~Light() {

}


glm::mat4 Light::getMVPMatrix( const glm::mat4 &_model, float _area) {
    // TODO:
    //      - Extend this to match different light types and not just directional

    if (bChange) {
        // From http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping
        _area *= 2.0;

        // Compute the MVP matrix from the light's point of view
        glm::mat4 depthProjectionMatrix = glm::ortho<float>(-_area, _area, -_area, _area, -_area, _area);
        glm::mat4 depthViewMatrix = glm::lookAt(glm::normalize(getPosition()), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
        m_mvp = depthProjectionMatrix * depthViewMatrix * _model;

        // Make biased MVP matrix (0 ~ 1) instad of (-1 to 1)
        glm::mat4 biasMatrix(
            0.5, 0.0, 0.0, 0.0,
            0.0, 0.5, 0.0, 0.0,
            0.0, 0.0, 0.5, 0.0,
            0.5, 0.5, 0.5, 1.0
        );
        m_mvp_biased = biasMatrix * m_mvp;

        bChange = false;
    }

    return m_mvp;
}

glm::mat4 Light::getBiasMVPMatrix() {
    return m_mvp_biased;
}

void Light::bindShadowMap() {
    if (m_shadowMap.getDepthTextureId() == 0) {
        #if defined(PLATFORM_RPI) || defined(PLATFORM_RPI4) 
        m_shadowMap.allocate(512, 512, DEPTH_TEXTURE);
        #else
        // m_shadowMap.allocate(1024, 1024, DEPTH_TEXTURE);
        m_shadowMap.allocate(1024, 1024, COLOR_DEPTH_TEXTURES);
        #endif
    }

    m_shadowMap.bind();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

void Light::unbindShadowMap() {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    m_shadowMap.unbind();
}
