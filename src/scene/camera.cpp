#include "camera.h"
#include "../window.h"

#include "glm/gtc/matrix_inverse.hpp"

// static const float MIN_APERTURE = 0.5f;
// static const float MAX_APERTURE = 64.0f;
// static const float MIN_SHUTTER_SPEED = 1.0f / 25000.0f;
// static const float MAX_SHUTTER_SPEED = 60.0f;
// static const float MIN_SENSITIVITY = 10.0f;
// static const float MAX_SENSITIVITY = 204800.0f;

Camera::Camera(): 
    m_target(0.0), 
    m_aspect(4.0f/3.0f), m_fov(45.), m_nearClip(0.01f), m_farClip(1000.0f), 
    m_exposure(2.60417e-05), m_ev100(14.9658), m_aperture(16), m_shutterSpeed(1.0f/125.0f), m_sensitivity(100.0f), 
    m_type(CameraType::PERSPECTIVE) {
    updateCameraSettings();
}

Camera::~Camera() {

}

void Camera::setViewport(int _width, int _height){
    m_aspect = double(_width) / double(_height);
    updateCameraSettings();
}

//Setting Functions
void Camera::setType(CameraType _type) {
    m_type = _type;
    m_viewMatrix = getTransformMatrix();
    lookAt(m_target);
    updateCameraSettings();
}

void Camera::setFOV(double _fov) {
    m_fov = _fov;
    updateCameraSettings();
}

void Camera::setClipping(double _near_clip_distance, double _far_clip_distance) {
    m_nearClip = _near_clip_distance;
    m_farClip = _far_clip_distance;
    updateCameraSettings();
}

void Camera::setTarget(glm::vec3 _target) {
    m_target = _target;
    bChange = true;
}

void Camera::setDistance(float _distance) {
    setPosition( -_distance * getZAxis() );
    lookAt(m_target);
}

void Camera::setVirtualOffset(float scale, int currentViewIndex, int totalViews) {
    // The standard model Looking Glass screen is roughly 4.75" vertically. If we
    // assume the average viewing distance for a user sitting at their desk is
    // about 36", our field of view should be about 14°. There is no correct
    // answer, as it all depends on your expected user's distance from the Looking
    // Glass, but we've found the most success using this figure.

    // start at -viewCone * 0.5 and go up to viewCone * 0.5
    const float viewCone = glm::radians(40.0);   // view cone of hardware, always around 40
    float offsetAngle = (float(currentViewIndex) / (float(totalViews) - 1.0f) - 0.5f) * viewCone;

    // calculate the offset that the camera should move
    float offset = -getDistance() * tan(offsetAngle) * 0.5f;

    // modify the view matrix (position)
    // determine the local direction of the offset 
    glm::vec3 offsetLocal = getXAxis() * offset;
    m_viewMatrix = glm::translate(getTransformMatrix(), offsetLocal);

    const float fov =  glm::radians(14.0f);
    glm::mat4 projectionMatrix = glm::perspective(fov, getAspect(), getNearClip(), getFarClip());
    // modify the projection matrix, relative to the camera size and aspect ratio

    float aspectRatio =  (float)getWindowWidth()/(float)getWindowHeight();
    projectionMatrix[2][0] += offset / (scale * aspectRatio);

    m_projectionMatrix = projectionMatrix;
    m_projectionViewMatrix = projectionMatrix * m_viewMatrix;
    m_normalMatrix = glm::transpose(glm::inverse(glm::mat3(m_viewMatrix)));
    bChange = true;

    // updateProjectionViewMatrix();
}

const glm::mat4& Camera::getViewMatrix() const {
    if (m_type == CameraType::PERSPECTIVE_VIRTUAL_OFFSET )
        return m_viewMatrix;
    else 
        return getTransformMatrix(); 
}

glm::vec3 Camera::getPosition() const {
    if (m_type == CameraType::PERSPECTIVE_VIRTUAL_OFFSET )
        return -glm::vec3(glm::inverse(m_viewMatrix)[3]);
    else 
        return m_position;
}

const float Camera::getDistance() const { 
    return glm::length(m_position);
}

/** Sets this camera's exposure (default is 16, 1/125s, 100 ISO)
 * from https://github.com/google/filament/blob/master/filament/src/Exposure.cpp
 *
 * The exposure ultimately controls the scene's brightness, just like with a real camera.
 * The default values provide adequate exposure for a camera placed outdoors on a sunny day
 * with the sun at the zenith.
 *
 * @param aperture      Aperture in f-stops, clamped between 0.5 and 64.
 *                      A lower \p aperture value *increases* the exposure, leading to
 *                      a brighter scene. Realistic values are between 0.95 and 32.
 *
 * @param shutterSpeed  Shutter speed in seconds, clamped between 1/25,000 and 60.
 *                      A lower shutter speed increases the exposure. Realistic values are
 *                      between 1/8000 and 30.
 *
 * @param sensitivity   Sensitivity in ISO, clamped between 10 and 204,800.
 *                      A higher \p sensitivity increases the exposure. Realistice values are
 *                      between 50 and 25600.
 *
 * @note
 * With the default parameters, the scene must contain at least one Light of intensity
 * similar to the sun (e.g.: a 100,000 lux directional light).
 *
 * @see Light, Exposure
 */

void  Camera::setExposure(float _aperture, float _shutterSpeed, float _sensitivity) {
    m_aperture = _aperture;
    m_shutterSpeed = _shutterSpeed;
    m_sensitivity = _sensitivity;
    
    // With N = aperture, t = shutter speed and S = sensitivity,
    // we can compute EV100 knowing that:
    //
    // EVs = log2(N^2 / t)
    // and
    // EVs = EV100 + log2(S / 100)
    //
    // We can therefore find:
    //
    // EV100 = EVs - log2(S / 100)
    // EV100 = log2(N^2 / t) - log2(S / 100)
    // EV100 = log2((N^2 / t) * (100 / S))
    //
    // Reference: https://en.wikipedia.org/wiki/Exposure_value
    m_ev100 = std::log2((_aperture * _aperture) / _shutterSpeed * 100.0f / _sensitivity);

    // This is equivalent to calling exposure(ev100(N, t, S))
    // By merging the two calls we can remove extra pow()/log2() calls
    const float e = (_aperture * _aperture) / _shutterSpeed * 100.0f / _sensitivity;
    m_exposure = 1.0f / (1.2f * e);

    bChange = true;
};

// ---------------------------------------------------------- Convertions

void Camera::updateCameraSettings() {
    setExposure(getAperture(), getShutterSpeed(), getSensitivity());
    
    if (m_type == CameraType::ORTHO)
        m_projectionMatrix = glm::ortho(-1.5f * float(m_aspect), 1.5f * float(m_aspect), -1.5f, 1.5f, -10.0f, 10.f);
    else
        m_projectionMatrix = glm::perspective(m_fov, m_aspect, m_nearClip, m_farClip);
    
    updateProjectionViewMatrix();
}

void Camera::updateProjectionViewMatrix() {
    m_projectionViewMatrix = m_projectionMatrix * getViewMatrix();
    m_normalMatrix = glm::transpose(glm::inverse(glm::mat3(getViewMatrix())));
    bChange = true;
}

glm::vec3 Camera::worldToCamera(glm::vec3 _WorldXYZ) const {
    glm::mat4 MVPmatrix = m_projectionViewMatrix;

    {
        MVPmatrix = glm::scale(glm::mat4(1.0), glm::vec3(1.f,-1.f,1.f)) * MVPmatrix;
    }

    glm::vec4 camera = MVPmatrix * glm::vec4(_WorldXYZ, 1.0);
    return glm::vec3(camera) / camera.w;
}

glm::vec3 Camera::worldToScreen(glm::vec3 _WorldXYZ) const {
    glm::vec3 CameraXYZ = worldToCamera(_WorldXYZ);

    glm::vec3 ScreenXYZ;
    ScreenXYZ.x = (CameraXYZ.x + 1.0f) * 0.5f;// * viewport.width + viewport.x;
    ScreenXYZ.y = (1.0f - CameraXYZ.y) * 0.5f;// * viewport.height + viewport.y;
    ScreenXYZ.z = CameraXYZ.z;

    return ScreenXYZ;
}

// ---------------------------------------------------------- Events

void Camera::onPositionChanged() {
    updateProjectionViewMatrix();
}

void Camera::onOrientationChanged() {
    updateProjectionViewMatrix();
}

void Camera::onScaleChanged() {
    updateProjectionViewMatrix();
}