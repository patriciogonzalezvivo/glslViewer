#include "polarPoint.h"

PolarPoint::PolarPoint():a(0.0),r(0.0),z(0.0){
}

PolarPoint::PolarPoint(const float &_angle, const float &_radius,const float &_z):a(_angle),r(_radius),z(_z){
}

PolarPoint::PolarPoint(const glm::vec3 &_org, const glm::vec3 &_dst):a(0.0),r(0.0),z((_dst.z+_org.z)*0.5){
    glm::vec3 diff = _dst - _org;
    a = atan2(diff.y,diff.x);
    r = glm::length(glm::vec2(diff.x,diff.y));
};

glm::vec3 PolarPoint::getXY(){
    return glm::vec3(r*cos(a), r*sin(a), z);
}
