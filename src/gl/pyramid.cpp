#include "pyramid.h"
#include <algorithm>
#include <iostream>
#include "gl.h"

Pyramid::Pyramid(): m_depth(0) {
}

Pyramid::~Pyramid() {
}

void Pyramid::allocate(int _width, int _height) {
    m_depth = log2(std::min(_width, _height)) - 1;
    m_depth = std::min(PYRAMID_MAX_LAYERS, m_depth);

    float w = _width;
    float h = _height;

    for (int i = 0; i < m_depth; i++) {
        w *= 0.5f;
        h *= 0.5f;
        m_downs[i].allocate(w, h, COLOR_TEXTURE);
        // std::cout << w << "x" << h << std::endl;
    }
    
    for (int i = 0; i < m_depth; i++) {
        w *= 2.0f;
        h *= 2.0f;
        m_ups[i].allocate(w, h, COLOR_TEXTURE);
        // std::cout << w << "x" << h << std::endl;
    }
}

void Pyramid::process(const Fbo *_input) {
    int i;
    pass(&m_downs[0], _input, NULL);

    for (i = 1; i < m_depth; i++)
        pass(&m_downs[i], &(m_downs[i-1]), NULL);
    
    pass(&m_ups[0], &(m_downs[m_depth-2]), &(m_downs[m_depth-1]));
    
    for (i = 1; i < m_depth-1; i++)
        pass(&m_ups[i], &(m_downs[m_depth-i-2]),&(m_ups[i-1]));
    
    pass(&m_ups[m_depth-1], _input, &(m_ups[m_depth-2]));
}

const Fbo* Pyramid::getResult(unsigned int index) const { 
    if (index < m_depth)
        return &m_ups[m_depth - 1 - index];
    else
        return &m_downs[m_depth * 2 - index - 1];
}