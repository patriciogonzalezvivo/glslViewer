#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

enum class CameraType {
    ORTHO, FREE
};

enum class CameraDirection {
    UP, DOWN, LEFT, RIGHT, FORWARD, BACK
};

class Camera {
public:
    Camera();
    virtual ~Camera();

    void update();

    void reset();

    //Given a specific moving direction, the camera will be moved in the appropriate direction
    //For a spherical camera this will be around the look_at point
    //For a free camera a delta will be computed for the direction of movement.
    void moveTo(CameraDirection dir);
    //Change the pitch (up, down) for the free camera
    void pitchTo(float degrees);
    //Change heading (left, right) for the free camera
    void headTo(float degrees);

    //Change the heading and pitch of the camera based on the 2d movement of the mouse
    void moveTo(float velX, float velY);

    //Setting Functions
    //Changes the camera mode, only three valid modes, Ortho, Free, and Spherical
    void setMode(CameraType cam_mode);
    //Set the position of the camera
    void setPosition(glm::vec3 pos);
    //Set's the look at point for the camera
    void setLookAt(glm::vec3 pos);
    //Changes the Field of View (FOV) for the camera
    void setFOV(double fov);
    //Change the viewport location size
    void setViewport(int width, int height);
    //Change the clipping distance for the camera
    void setClipping(double near_clip_distance, double far_clip_distance);
    void setDistance(double cam_dist);

    //Getting Functions
    CameraType getType();
    double getDistance();
    glm::mat4 getProjectionViewMatrix();

protected:
    glm::mat4 m_projectionViewMatrix;

    glm::mat4 m_projection;
    glm::mat4 m_view;

    glm::quat m_rotation;
    glm::vec3 m_upVector;

    glm::vec3 m_position;
    glm::vec3 m_velocity;
    glm::vec3 m_lookAt;
    glm::vec3 m_direction;

    double m_aspect;
    double m_fov;
    double m_nearClip;
    double m_farClip;

    float m_scale;
    float m_heading;
    float m_pitch;

    float max_pitch_rate;
    float max_heading_rate;

    CameraType m_type;
};