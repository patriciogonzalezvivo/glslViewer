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
    CameraType  getType();
    glm::mat4   getProjectionMatrix();
    glm::mat4   getProjectionViewMatrix();

protected:

    virtual void onPositionChanged();
    virtual void onOrientationChanged();
    virtual void onScaleChanged();

    void    updateProjectionViewMatrix();

private:
    glm::mat4 m_projectionViewMatrix;

    glm::mat4 m_projection;

    double m_aspect;
    double m_fov;
    double m_nearClip;
    double m_farClip;

    CameraType m_type;
};
