#pragma once

#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"

class Node {
public:

    Node();
    virtual ~Node();

    void    setTransformMatrix(const glm::mat4& _m);
    void    setPosition(const glm::vec3& _pos);
    void    setOrientation(const glm::vec3& _ori);
    void    setOrientation(const glm::quat& _ori);
    void    setScale(const glm::vec3& _scale);

    glm::vec3   getXAxis() const;
    glm::vec3   getYAxis() const;
    glm::vec3   getZAxis() const;

    glm::vec3   getPosition() const;
    glm::vec3   getLookAtDir() const;
    glm::vec3   getUpDir() const;

    float   getPitch() const;
    float   getHeading() const;
    float   getRoll() const;

    glm::quat   getOrientationQuat() const;
    glm::vec3   getOrientationEuler() const;
    glm::vec3   getScale() const;

    const glm::mat4& getTransformMatrix() const;

    void    move(const glm::vec3& _offset);

    void    truck(float _amount);
    void    boom(float _amount);
    void    dolly(float _amount);

    void    tilt(float _degrees);
    void    pan(float _degrees);
    void    roll(float _degrees);

    void    rotate(const glm::quat& _q);

    void    rotateAround(const glm::quat& _q, const glm::vec3& _point);

    void    lookAt(const glm::vec3& _lookAtPosition, glm::vec3 _upVector = glm::vec3(0.0, 1.0, 0.0));

    void    orbit(float _longitude, float _latitude, float _radius, const glm::vec3& _centerPoint = glm::vec3(0.0));

    void    reset();

protected:

    void    createMatrix();
    void    updateAxis();

    virtual void onPositionChanged() {};
    virtual void onOrientationChanged() {};
    virtual void onScaleChanged() {};

private:

    glm::mat4   m_transformMatrix;
    glm::vec3   m_axis[3];

    glm::vec3   m_position;
    glm::quat   m_orientation;
    glm::vec3   m_scale;

};
