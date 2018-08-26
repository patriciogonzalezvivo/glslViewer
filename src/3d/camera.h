#pragma once

#include "node.h"

enum class CameraType {
    ORTHO, FREE
};

class Camera : public Node {
public:
    Camera();
    virtual ~Camera();

    void    setMode(CameraType cam_mode);
    void    setFOV(double _fov);
    void    setViewport(int _width, int _height);
    void    setClipping(double _near_clip_distance, double _far_clip_distance);
    void    setDistance(float _distance);
    void    setTarget(glm::vec3 _target);

    glm::vec3   worldToCamera(glm::vec3 _WorldXYZ) const;
    glm::vec3   worldToScreen(glm::vec3 _WorldXYZ) const;

    //Getting Functions
    float               getDistance() const { return glm::length(getPosition()); }
    const CameraType&   getType() const;
    const glm::mat3&    getNormalMatrix() const;
    const glm::mat4&    getViewMatrix() const { return getTransformMatrix(); }
    const glm::mat4&    getProjectionMatrix() const;
    const glm::mat4&    getProjectionViewMatrix() const;

    float   exposure; 
    float   ev100;

protected:

    virtual void onPositionChanged();
    virtual void onOrientationChanged();
    virtual void onScaleChanged();

    void    updateCameraSettings();
    void    updateProjectionViewMatrix();

private:
    glm::mat4 m_projectionViewMatrix;

    glm::mat4 m_projectionMatrix;
    glm::mat4 m_viewMatrix;
    glm::mat3 m_normalMatrix;

    glm::vec3 m_target;

    double m_aspect;
    double m_fov;
    double m_nearClip;
    double m_farClip;

    CameraType m_type;
};
