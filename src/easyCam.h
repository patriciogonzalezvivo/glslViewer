#pragma once

#include "camera.h"

class EasyCam : public Camera {
public:
    EasyCam();
    virtual ~EasyCam();

    void setDistance(float _distance);

    float getDistance() const;
private:

};