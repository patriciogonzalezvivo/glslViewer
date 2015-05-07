#include "EasyCam.h"

EasyCam::EasyCam(){

}

EasyCam::~EasyCam(){

}

void EasyCam::setDistance(float _distance){
    if(_distance > 0.0f){
        setPosition( -_distance * getZAxis() );
    }
}

float EasyCam::getDistance() const {
    return glm::length(getPosition());
}