#include "pingpong.h"
#include <iostream>

PingPong::PingPong() {
}

PingPong::~PingPong() {
}

void PingPong::allocate(int _width, int _height) {
    for(int i = 0; i < 2; i++){
        m_fbos[i].allocate(_width, _height);
    }
    
    // Set everything to 0
    m_flag = 0;
    swap();
}
    
void PingPong::swap(){
    src = &(m_fbos[(m_flag)%2]);
    dst = &(m_fbos[++(m_flag)%2]);
}
