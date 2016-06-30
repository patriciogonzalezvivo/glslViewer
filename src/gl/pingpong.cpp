#include "pingpong.h"
#include <iostream>

PingPong::PingPong() {
    std::cout << "Create PingPong" << std::endl;
}

PingPong::~PingPong() {
    
}

void PingPong::allocate(int _width, int _height) {
    for(int i = 0; i < 2; i++){
        m_fbos[i].allocate(_width, _height);
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
