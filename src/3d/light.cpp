#include "light.h"

Light::Light(): color(1.0), bChange(true), m_mvp(1.0), m_mvp_biased(1.0), m_type(LIGHT_DIRECTIONAL) {

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