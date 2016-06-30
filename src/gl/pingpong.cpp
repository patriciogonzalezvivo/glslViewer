#pragma once

#include "pingpong.h"

PingPong::PingPong():m_flag(0) {

}

PingPong::~PingPong() {
    
}

void PingPong::resize(int _width, int _height) {
    for(int i = 0; i < 2; i++){
        m_fbos[i].resize(_width, _height);
    }

    clear();
    
    // Set everything to 0
    m_flag = 0;
    swap();
    m_flag = 0;
}
    
void PingPong::swap(){
    src = &(m_fbos[(m_flag)%2]);
    dst = &(m_fbos[++(m_flag)%2]);
}
    
void PingPong::clear(float _alpha) {
    for(int i = 0; i < 2; i++){
        m_fbos[i].clear(_alpha);
    }
}
