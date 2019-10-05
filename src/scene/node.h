#pragma once

#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"

class Node {
public:

    Node();
    virtual ~Node();

    virtual void        setProperties(const Node& _other);

    virtual void        setTransformMatrix(const glm::mat4& _m);
    virtual void        setPosition(const glm::vec3& _pos);
    virtual void        setOrientation(const glm::vec3& _ori);
    virtual void        setOrientation(const glm::quat& _ori);
    virtual void        setScale(const glm::vec3& _scale);

    virtual glm::vec3   getXAxis() const;
    virtual glm::vec3   getYAxis() const;
    virtual glm::vec3   getZAxis() const;

    virtual glm::vec3   getPosition() const;
    virtual glm::vec3   getLookAtDir() const;
    virtual glm::vec3   getUpDir() const;

    virtual float       getPitch() const;
    virtual float       getHeading() const;
    virtual float       getRoll() const;

    virtual glm::quat   getOrientationQuat() const;
    virtual glm::vec3   getOrientationEuler() const;
    virtual glm::vec3   getScale() const;

    virtual const glm::mat4& getTransformMatrix() const;

    virtual void        scale(const glm::vec3& _scale);
    virtual void        translate(const glm::vec3& _offset);

    virtual void        truck(float _amount);
    virtual void        boom(float _amount);
    virtual void        dolly(float _amount);

    virtual void        tilt(float _degrees);
    virtual void        pan(float _degrees);
    virtual void        roll(float _degrees);

    virtual void        rotate(const glm::quat& _q);
    virtual void        rotateAround(const glm::quat& _q, const glm::vec3& _point);
    virtual void        lookAt(const glm::vec3& _lookAtPosition, glm::vec3 _upVector = glm::vec3(0.0, 1.0, 0.0));
    virtual void        orbit(float _longitude, float _latitude, float _radius, const glm::vec3& _centerPoint = glm::vec3(0.0));

    virtual void        apply(const glm::mat4& _m);

    virtual void        reset();

    bool    bChange;

protected:
    virtual void        createMatrix();
    virtual void        updateAxis();

    virtual void        onPositionChanged() {};
    virtual void        onOrientationChanged() {};
    virtual void        onScaleChanged() {};

private:
    glm::mat4           m_transformMatrix;
    glm::vec3           m_axis[3];

    glm::vec3           m_position;
    glm::quat           m_orientation;
    glm::vec3           m_scale;
};
