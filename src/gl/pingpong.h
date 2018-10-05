#pragma once

#include "fbo.h"

class PingPong {
public:
    PingPong();
    virtual ~PingPong();

    void allocate(int _width, int _height, FboType _type);
    void swap();
    void clear(float _alpha = 0.0);

    Fbo& operator[](int n){ return m_fbos[n]; }

    Fbo *src;       // Source       ->  Ping
    Fbo *dst;       // Destination  ->  Pong

private:
    Fbo m_fbos[2];    // Real addresses of ping/pong FBO´s
    int m_flag;       // Integer for making a quick swap
};
