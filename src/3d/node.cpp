#include "node.h"

#include "glm/gtx/matrix_decompose.hpp"
#include "glm/gtc/matrix_access.hpp"

#include "tools/geom.h"

Node::Node() {
    setPosition(glm::vec3(0.0));
    setOrientation(glm::vec3(0.0));
    setScale(glm::vec3(1.));
}
Node::~Node() {

}

void Node::setTransformMatrix(const glm::mat4& _m) {
    m_transformMatrix = _m;

    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(m_transformMatrix, m_scale, m_orientation, m_position, skew, perspective );

    updateAxis();

    onPositionChanged();
    onOrientationChanged();
    onScaleChanged();
}

void Node::setPosition(const glm::vec3& _pos) {
    m_position = _pos;
    // m_transformMatrix = glm::translate(m_transformMatrix,_pos);
    m_transformMatrix[3][0] = _pos.x;
    m_transformMatrix[3][1] = _pos.y;
    m_transformMatrix[3][2] = _pos.z;
    onPositionChanged();
}

void Node::setOrientation(const glm::vec3& _ori){
    glm::quat x = glm::angleAxis(_ori.x, glm::vec3(1.0,0.0,0.0));
    glm::quat y = glm::angleAxis(_ori.y, glm::vec3(0.0,1.0,0.0));
    glm::quat z = glm::angleAxis(_ori.z, glm::vec3(0.0,0.0,1.0));
    setOrientation(x*y*z);
}

void Node::setOrientation(const glm::quat& _ori) {
    m_orientation = _ori;
    createMatrix();
    onOrientationChanged();
}

void Node::setScale(const glm::vec3& _scale) {
    m_scale = _scale;
    createMatrix();
    onScaleChanged();
}

glm::vec3 Node::getXAxis() const {
    return m_axis[0];
}

glm::vec3 Node::getYAxis() const {
    return m_axis[1];
}

glm::vec3 Node::getZAxis() const{
    return m_axis[2];
}

glm::vec3 Node::getPosition() const {
    return m_position;
}

glm::vec3 Node::getLookAtDir() const{
    return -getZAxis();
}

glm::vec3 Node::getUpDir() const {
    return getYAxis();
}

float Node::getPitch() const {
    return getOrientationEuler().x;
}

float Node::getHeading() const {
    return getOrientationEuler().y;
}

float Node::getRoll() const {
    return getOrientationEuler().z;
}

glm::quat Node::getOrientationQuat() const {
    return m_orientation;
}

glm::vec3 Node::getOrientationEuler() const {
    return glm::eulerAngles(m_orientation);
}

glm::vec3 Node::getScale() const {
    return m_scale;
}

const glm::mat4& Node::getTransformMatrix() const {
    return m_transformMatrix;
}

void Node::move(const glm::vec3& _offset) {
    m_position += _offset;
    m_transformMatrix = glm::translate(m_transformMatrix, m_position);
    onPositionChanged();
}

void Node::truck(float _amount) {
    move(getXAxis() * _amount);
}

void Node::boom(float _amount) {
    move(getYAxis() * _amount);
}

void Node::dolly(float _amount) {
    move(getZAxis() * _amount);
}

void Node::tilt(float _degrees) {
    rotate(glm::angleAxis(glm::radians(_degrees), getXAxis()));
}

void Node::pan(float _degrees) {
    rotate(glm::angleAxis(glm::radians(_degrees), getYAxis()));
}

void Node::roll(float _degrees) {
    rotate(angleAxis(glm::radians(_degrees), getZAxis()));
}

void Node::rotate(const glm::quat& _q) {
    m_orientation *= _q;

    createMatrix();
    onOrientationChanged();
}

void Node::rotateAround(const glm::quat& _q, const glm::vec3& _point) {
    setPosition( (getPosition() - _point) * _q + _point);

    onOrientationChanged();
    onPositionChanged();
}

void Node::lookAt(const glm::vec3& _lookAtPosition, glm::vec3 _upVector ) {
    glm::mat4 m = glm::lookAt(-getPosition(), _lookAtPosition, _upVector);
    setOrientation(glm::toQuat(m));
}

void Node::orbit(float _lat, float _lon, float _radius, const glm::vec3& _centerPoint) {
    glm::vec3 p = glm::vec3(0.0, 0.0, _radius);

    _lon = CLAMP(_lon,-89.9,89.9);

    glm::quat lat = glm::angleAxis(glm::radians(_lon), glm::vec3(1.0, 0.0, 0.0));
    glm::quat lon = glm::angleAxis(glm::radians(_lat), glm::vec3(0.0, 1.0, 0.0));
    p = lat * p;
    p = lon * p;
    p += _centerPoint;
    setPosition(p);

    lookAt(_centerPoint);
}

void Node::reset(){
    setPosition(glm::vec3(0.0));
    setOrientation(glm::vec3(0.0));
}

void Node::createMatrix() {
    m_transformMatrix = glm::scale(m_scale);
    m_transformMatrix = glm::toMat4(m_orientation)*m_transformMatrix;
    m_transformMatrix = glm::translate(m_transformMatrix, m_position);

    updateAxis();
}

void Node::updateAxis() {
    if(m_scale[0]>0) {
        m_axis[0] = glm::vec3(glm::row(getTransformMatrix(),0))/m_scale[0];
    }

    if(m_scale[1]>0) {
        m_axis[1] = glm::vec3(glm::row(getTransformMatrix(),1))/m_scale[1];
    }

    if(m_scale[2]>0) {
        m_axis[2] = glm::vec3(glm::row(getTransformMatrix(),2))/m_scale[2];
    }
}
