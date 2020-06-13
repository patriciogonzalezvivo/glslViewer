#include "geom.h"

#include <algorithm>
#include <iostream>

void getBoundingBox(const std::vector<glm::vec3> &_pts, glm::vec3 &_min, glm::vec3 &_max) {
    _min = glm::vec3(10000.0, 10000.0, 10000.0);
    _max = glm::vec3(-10000.0, -10000.0, -10000.0);

    return expandBoundingBox(_pts, _min, _max);
}

void expandBoundingBox(const std::vector<glm::vec3> &_pts, glm::vec3 &_min, glm::vec3 &_max) {
    for (unsigned int i = 0; i < _pts.size(); i++) {
        expandBoundingBox(_pts[i], _min, _max);
    }
}

void expandBoundingBox(const glm::vec3 &_pt, glm::vec3 &_min, glm::vec3 &_max) {
    if ( _pt.x < _min.x)
        _min.x = _pt.x;

    if ( _pt.y < _min.y)
        _min.y = _pt.y;

    if ( _pt.z < _min.z)
        _min.z = _pt.z;

    if ( _pt.x > _max.x)
        _max.x = _pt.x;

    if ( _pt.y > _max.y)
        _max.y = _pt.y;

    if ( _pt.z > _max.z)
        _max.z = _pt.z;
}

glm::vec3 getCentroid(const std::vector<glm::vec3> &_pts) {
    glm::vec3 centroid;
    for (uint32_t i = 0; i < _pts.size(); i++) {
        centroid += _pts[i] / (float)_pts.size();
    }
    return centroid;
}

void calcNormal(const glm::vec3& _v0, const glm::vec3& _v1, const glm::vec3& _v2, glm::vec3& _N) {
    glm::vec3 v10 = _v1 - _v0;
    glm::vec3 v20 = _v2 - _v0;

    _N.x = v20.x * v10.z - v20.z * v10.y;
    _N.y = v20.z * v10.x - v20.x * v10.z;
    _N.z = v20.x * v10.y - v20.y * v10.x;
    
    _N = glm::normalize(_N);
}
