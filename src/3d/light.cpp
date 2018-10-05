#include "light.h"

Light::Light(): color(1.0), bChange(true), m_type(LIGHT_DIRECTIONAL) {

}

Light::~Light() {

}


glm::mat4 Light::getMVPMatrix( glm::mat4 _model ) {

    // From http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping

    // Compute the MVP matrix from the light's point of view
    glm::mat4 depthProjectionMatrix = glm::ortho<float>(-25.0, 25.0, -25.0, 25.0, -25.0, 25.0);
    glm::mat4 depthViewMatrix = glm::lookAt(glm::normalize(getPosition()), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
    glm::mat4 depthMVP = depthProjectionMatrix * depthViewMatrix * _model;


    // TODO:
    //      - Extend this to match different light types and not just directional
    
    return depthMVP;
}