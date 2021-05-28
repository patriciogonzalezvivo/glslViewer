#include "pyramid.h"
#include <algorithm>
#include <iostream>
#include "gl.h"

Pyramid::Pyramid(): m_width(0), m_height(0), m_depth(0) {
}

Pyramid::~Pyramid() {
}

void Pyramid::allocate(int _width, int _height) {
    m_depth = log2(std::min(_width, _height)) - 1;
    m_depth = std::min(PYRAMID_MAX_LAYERS, m_depth);
    m_width = _width;
    m_height = _height;
    std::cout << m_depth << std::endl;

    for (int i = 0; i < m_depth; i++) {
        _width /= 2;
        _height /= 2;
        m_downs[i].allocate(_width, _height, COLOR_TEXTURE);
    }
    
    for (int i = 0; i < m_depth; i++) {
        _width *= 2;
        _height *= 2;
        m_ups[i].allocate(_width, _height, COLOR_TEXTURE);
    }
}

void Pyramid::process(const Fbo *_input) {
    int i;
    pass(&m_downs[0], _input, NULL);

    for (i = 1; i < m_depth; i++)
        pass(&m_downs[i], &(m_downs[i-1]),NULL);
    
    pass(&m_ups[0], &(m_downs[m_depth-2]), &(m_downs[m_depth-1]));
    
    for (i = 1; i < m_depth-1; i++)
        pass(&m_ups[i], &(m_downs[m_depth-i-2]),&(m_ups[i-1]));
    
    pass(&m_ups[m_depth-1], _input, &(m_ups[m_depth-2]));
}