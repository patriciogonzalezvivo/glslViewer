#include "camera.h"

#include "glm/gtc/matrix_inverse.hpp"

Camera::Camera(): exposure(1.0), ev100(1.0), m_target(0.0), m_aspect(4.0f/3.0f), m_fov(45.), m_nearClip(0.01f),m_farClip(100.0f), m_type(CameraType::FREE){
    updateCameraSettings();
}

Camera::~Camera() {

}

void Camera::setViewport(int _width, int _height){
    m_aspect = double(_width) / double(_height);
    updateCameraSettings();
}

//Setting Functions
void Camera::setMode(CameraType _type) {
    m_type = _type;
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
}

void Camera::setDistance(float _distance) {
    setPosition( -_distance * getZAxis() );
    lookAt(m_target);
}

const CameraType& Camera::getType() const {
    return m_type;
}

const glm::mat3& Camera::getNormalMatrix() const {
    return m_normalMatrix;
}

const glm::mat4& Camera::getProjectionMatrix() const {
    return m_projectionMatrix;
}

const glm::mat4& Camera::getProjectionViewMatrix() const {
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

void Camera::updateCameraSettings() {
    if (m_type == CameraType::ORTHO) {
        m_projectionMatrix = glm::ortho(-1.5f * float(m_aspect), 1.5f * float(m_aspect), -1.5f, 1.5f, -10.0f, 10.f);
    }
    else if (m_type == CameraType::FREE) {
        m_projectionMatrix = glm::perspective(m_fov, m_aspect, m_nearClip, m_farClip);
    }
    updateProjectionViewMatrix();
}

void Camera::updateProjectionViewMatrix() {
    m_projectionViewMatrix = m_projectionMatrix * getViewMatrix();
    m_normalMatrix = glm::transpose(glm::inverse(glm::mat3(getViewMatrix())));
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