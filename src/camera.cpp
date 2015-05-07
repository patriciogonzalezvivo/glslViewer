#include "camera.h"

Camera::Camera(): m_aspect(4.0f/3.0f), m_fov(45.), m_nearClip(0.01f),m_farClip(100.0f), m_type(CameraType::FREE) {

}

Camera::~Camera() {

}

void Camera::setViewport(int _width, int _height){
    m_aspect = double(_width) / double(_height);
    updateProjectionViewMatrix();
}

//Setting Functions
void Camera::setMode(CameraType _type) {
    m_type = _type;
    lookAt(glm::vec3(0.0,0.0,0.0));
}

void Camera::setFOV(double _fov) {
    m_fov = _fov;
}

void Camera::setClipping(double _near_clip_distance, double _far_clip_distance) {
    m_nearClip = _near_clip_distance;
    m_farClip = _far_clip_distance;
}

CameraType Camera::getType() {
    return m_type;
}

glm::mat4 Camera::getProjectionMatrix(){
    return m_projection;
}

glm::mat4 Camera::getProjectionViewMatrix() {
    return m_projectionViewMatrix;
}

void Camera::onPositionChanged() {
    updateProjectionViewMatrix();
}

void Camera::onOrientationChanged() {
    updateProjectionViewMatrix();
}

void Camera::onScaleChanged() {
    updateProjectionViewMatrix();
}

void Camera::updateProjectionViewMatrix() {
    if (m_type == CameraType::ORTHO) {
        m_projection = glm::ortho(-1.5f * float(m_aspect), 1.5f * float(m_aspect), -1.5f, 1.5f, -10.0f, 10.f);
    } else if (m_type == CameraType::FREE) {
        m_projection = glm::perspective(m_fov, m_aspect, m_nearClip, m_farClip);
    }

    m_projectionViewMatrix = m_projection * getTransformMatrix();
}