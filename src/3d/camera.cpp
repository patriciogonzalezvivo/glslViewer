/*
Copyright (c) 2014, Patricio Gonzalez Vivo
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "camera.h"

#include "glm/gtc/matrix_inverse.hpp"

Camera::Camera(): m_aspect(4.0f/3.0f), m_fov(45.), m_nearClip(0.01f),m_farClip(100.0f), m_type(CameraType::FREE) {
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
    lookAt(glm::vec3(0.0,0.0,0.0));
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
    } else if (m_type == CameraType::FREE) {
        m_projectionMatrix = glm::perspective(m_fov, m_aspect, m_nearClip, m_farClip);
    }
    updateProjectionViewMatrix();
}

void Camera::updateProjectionViewMatrix() {
    m_projectionViewMatrix = m_projectionMatrix * getViewMatrix();
    // m_normalMatrix = glm::inverseTranspose(glm::mat3(getViewMatrix()));
    m_normalMatrix = glm::transpose(glm::inverse(glm::mat3(getViewMatrix())));
}
