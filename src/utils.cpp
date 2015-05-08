#include "utils.h"

void wrapRad(double &_angle){
    if (_angle < -PI) _angle += PI*2.;
    if (_angle > PI) _angle -= PI*2.;
}

void wrapDeg(float &_angle){
    if (_angle < -180.0) _angle += 360.0;
    if (_angle > 180) _angle -= 360.0;
}

glm::vec3 getCentroid(const std::vector<glm::vec3> &_pts){
    glm::vec3 centroid;
    for (int i = 0; i < _pts.size(); i++) {
        centroid += _pts[i] / (float)_pts.size();
    }
    return centroid;
}