#include "camera.h"

Camera::Camera():m_projectionViewMatrix(1.), m_projection(1.), m_view(1.), 
    m_rotation(1., 0., 0., 0.), m_upVector(0., 1., 0.),
    m_position(0.,0.,-2.), m_velocity(0., 0., 0.), m_lookAt(0.,0.,0.), m_direction(0.,0.,1.), 
    m_aspect(4.0f/3.0f), m_fov(45.), m_nearClip(0.01f),m_farClip(100.0f), 
    m_scale(5.0f), m_heading(0.), m_pitch(0.),
    max_pitch_rate(0.005), max_heading_rate(0.005), m_type(CameraType::FREE) {

}

Camera::~Camera() {

}

void Camera::update() {
    m_direction = glm::normalize(m_lookAt - m_position);
    
    if (m_type == CameraType::ORTHO) {
        //our projection matrix will be an orthogonal one in this case
        //if the values are not floating point, this command does not work properly
        //need to multiply by aspect!!! (otherise will not scale properly)
        m_projection = glm::ortho(-1.5f * float(m_aspect), 1.5f * float(m_aspect), -1.5f, 1.5f, -10.0f, 10.f);
    } else if (m_type == CameraType::FREE) {
        m_projection = glm::perspective(m_fov, m_aspect, m_nearClip, m_farClip);
        //detmine axis for pitch rotation
        glm::vec3 axis = glm::cross(m_direction, m_upVector);
        //compute quaternion for pitch based on the camera pitch angle
        glm::quat pitch_quat = glm::angleAxis(m_pitch, axis);
        //determine heading quaternion from the camera up vector and the heading angle
        glm::quat heading_quat = glm::angleAxis(m_heading, m_upVector);
        //add the two quaternions
        glm::quat temp = glm::cross(pitch_quat, heading_quat);
        temp = glm::normalize(temp);
        //update the direction from the quaternion
        m_direction = glm::rotate(temp, m_direction);
        //add the camera delta
        m_position += m_velocity;
        //set the look at to be infront of the camera
        m_lookAt = m_position + m_direction * 1.0f;
        //damping for smooth camera
        m_heading *= .5;
        m_pitch *= .5;
        m_velocity = m_velocity * .8f;
    }
    //compute the MVP
    m_view = glm::lookAt(m_position, m_lookAt, m_upVector);

    m_projectionViewMatrix = m_projection * m_view;
}

void Camera::setViewport(int _width, int _height){
    m_aspect = double(_width) / double(_height);
}

void Camera::reset() {
    m_upVector = glm::vec3(0, 1, 0);
}

//Setting Functions
void Camera::setMode(CameraType _type) {
    m_type = _type;
    m_upVector = glm::vec3(0, 1, 0);
    m_rotation = glm::quat(1, 0, 0, 0);
}

void Camera::setPosition(glm::vec3 _pos) {
    m_position = _pos;
}

void Camera::setLookAt(glm::vec3 _pos) {
    m_lookAt = _pos;
}

void Camera::setFOV(double _fov) {
    m_fov = _fov;
}

void Camera::setClipping(double _near_clip_distance, double _far_clip_distance) {
    m_nearClip = _near_clip_distance;
    m_farClip = _far_clip_distance;
}

void Camera::setDistance(double _dist) {
    glm::vec3 newPosition = glm::normalize(m_position-m_lookAt);
    newPosition.x *= _dist;
    newPosition.y *= _dist;
    newPosition.z *= _dist;

    m_velocity = m_position - newPosition;
}

double Camera::getDistance(){
    return glm::length(m_position-m_lookAt);
}

void Camera::moveTo(CameraDirection _dir) {
    if (m_type == CameraType::FREE) {
        switch (_dir) {
            case CameraDirection::UP:
                m_velocity += m_upVector * m_scale;
                break;
            case CameraDirection::DOWN:
                m_velocity -= m_upVector * m_scale;
                break;
            case CameraDirection::LEFT:
                m_velocity -= glm::cross(m_direction, m_upVector) * m_scale;
                break;
            case CameraDirection::RIGHT:
                m_velocity += glm::cross(m_direction, m_upVector) * m_scale;
                break;
            case CameraDirection::FORWARD:
                m_velocity += m_direction * m_scale;
                break;
            case CameraDirection::BACK:
                m_velocity -= m_direction * m_scale;
                break;
        }
    }
}
void Camera::pitchTo(float _degrees) {
    //Check bounds with the max pitch rate so that we aren't moving too fast
    if (_degrees < -max_pitch_rate) {
        _degrees = -max_pitch_rate;
    } else if (_degrees > max_pitch_rate) {
        _degrees = max_pitch_rate;
    }
    m_pitch += _degrees;

    //Check bounds for the camera pitch
    if (m_pitch > 360.0f) {
        m_pitch -= 360.0f;
    } else if (m_pitch < -360.0f) {
        m_pitch += 360.0f;
    }
}
void Camera::headTo(float _degrees) {
    //Check bounds with the max heading rate so that we aren't moving too fast
    if (_degrees < -max_heading_rate) {
        _degrees = -max_heading_rate;
    } else if (_degrees > max_heading_rate) {
        _degrees = max_heading_rate;
    }
    //This controls how the heading is changed if the camera is pointed straight up or down
    //The heading delta direction changes
    if ( (m_pitch > 90 && m_pitch < 270) || (m_pitch < -90 && m_pitch > -270)) {
        m_heading -= _degrees;
    } else {
        m_heading += _degrees;
    }
    //Check bounds for the camera heading
    if (m_heading > 360.0f) {
        m_heading -= 360.0f;
    } else if (m_heading < -360.0f) {
        m_heading += 360.0f;
    }
}

CameraType Camera::getType() {
    return m_type;
}

glm::mat4 Camera::getProjectionViewMatrix() {
    return m_projectionViewMatrix;
}