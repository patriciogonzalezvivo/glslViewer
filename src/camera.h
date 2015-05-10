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
    void    setFOV(double fov);
    void    setViewport(int width, int height);
    void    setClipping(double near_clip_distance, double far_clip_distance);

    //Getting Functions
    const CameraType&   getType() const;
    const glm::mat3&    getNormalMatrix() const;
    const glm::mat4&    getViewMatrix() const { return getTransformMatrix(); }
    const glm::mat4&    getProjectionMatrix() const;
    const glm::mat4&    getProjectionViewMatrix() const;

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

    double m_aspect;
    double m_fov;
    double m_nearClip;
    double m_farClip;

    CameraType m_type;
};
