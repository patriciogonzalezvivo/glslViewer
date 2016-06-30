#pragma once

#include "fbo.h"

class PingPong {
public:
    virtual void resize( int _width, int _height){

        for(int i = 0; i < 2; i++){
            m_fbos[i].resize(_width, _height);
        }
    
        clear();
        
        // Set everything to 0
        flag = 0;
        swap();
        flag = 0;
    }
    
    virtual void swap(){
        src = &(m_fbos[(flag)%2]);
        dst = &(m_fbos[++(flag)%2]);
    }
    
    virtual void clear(float _alpha = 0.0) {
        for(int i = 0; i < 2; i++){
            m_fbos[i].clear(_alpha);
        }
    }
    
    Fbo& operator[]( int n ){ return m_fbos[n];}
    
    Fbo *src;       // Source       ->  Ping
    Fbo *dst;       // Destination  ->  Pong
    
private:
    Fbo m_fbos[2];    // Real addresses of ping/pong FBO´s  
    int flag;       // Integer for making a quick swap
};
