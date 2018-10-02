#pragma once

#include "node.h"

enum CameraType {
    ORTHO, FREE
};

class Camera: public Node {
public:
    Camera();
    virtual ~Camera();

    virtual void        setMode(CameraType cam_mode);
    virtual void        setFOV(double _fov);
    virtual void        setViewport(int _width, int _height);
    virtual void        setClipping(double _near_clip_distance, double _far_clip_distance);
    virtual void        setDistance(float _distance);
    virtual void        setTarget(glm::vec3 _target);

    virtual void        setExposure(float _aperture, float _shutterSpeed, float _sensitivity);

    virtual glm::vec3   worldToCamera(glm::vec3 _WorldXYZ) const;
    virtual glm::vec3   worldToScreen(glm::vec3 _WorldXYZ) const;

    //Getting Functions
    const float         getFOV() const { return m_fov; }
    const float         getFarClip() const { return m_farClip; }
    const float         getNearClip() const { return m_nearClip; }
    const float         getDistance() const { return glm::length(getPosition()); }

    const float         getEv100() const { return m_ev100; }
    const float         getExposure() const { return m_exposure; }
    const float         getAperture() const { return m_aperture; }          //! returns this camera's aperture in f-stops
    const float         getShutterSpeed() const { return m_shutterSpeed; }  //! returns this camera's shutter speed in seconds
    const float         getSensitivity() const { return m_sensitivity; }    //! returns this camera's sensitivity in ISO
    const glm::vec3     getTarget() const { return m_target; }
    
    const CameraType&   getType() const { return m_type;};
    const glm::mat3&    getNormalMatrix() const { return m_normalMatrix; }
    const glm::mat4&    getViewMatrix() const { return getTransformMatrix(); }
    const glm::mat4&    getProjectionMatrix() const { return m_projectionMatrix; }
    const glm::mat4&    getProjectionViewMatrix() const { return m_projectionViewMatrix; }

protected:

    virtual void        onPositionChanged();
    virtual void        onOrientationChanged();
    virtual void        onScaleChanged();

    virtual void        updateCameraSettings();
    virtual void        updateProjectionViewMatrix();

private:
    glm::mat4   m_projectionViewMatrix;

    glm::mat4   m_projectionMatrix;
    glm::mat4   m_viewMatrix;
    glm::mat3   m_normalMatrix;

    glm::vec3   m_target;

    double      m_aspect;
    double      m_fov;
    double      m_nearClip;
    double      m_farClip;

    float       m_exposure; 
    float       m_ev100;
    float       m_aperture;
    float       m_shutterSpeed;
    float       m_sensitivity;

    CameraType  m_type;
};
