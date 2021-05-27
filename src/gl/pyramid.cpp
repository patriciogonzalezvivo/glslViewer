#include "pyramid.h"
#include <algorithm>
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

void Pyramid::process(Vbo* vbo, const Fbo *_input) {
    int i;
    pass(vbo, &m_downs[0], _input, NULL);

    for (i = 1; i < m_depth; i++)
        pass(vbo, &m_downs[i], &(m_downs[i-1]),NULL);
    
    pass(vbo, &m_ups[0], &(m_downs[m_depth-2]), &(m_downs[m_depth-1]));
    
    for (i = 1; i < m_depth-1; i++)
        pass(vbo, &m_ups[i], &(m_downs[m_depth-i-2]),&(m_ups[i-1]));
    
    pass(vbo, &m_ups[m_depth-1], _input, &(m_ups[m_depth-2]));
}

void Pyramid::pass(Vbo* _vbo, Fbo *_target, const Fbo *_texUnf, const Fbo *_texFil) {
    _target->bind();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    shader.use();

    shader.setUniformTexture("u_texUnf", _texUnf, 1);
    if (_texFil != NULL){
      shader.setUniformTexture("u_texFil", _texFil, 2);
    }
    shader.setUniform("u_resolution", (float)_target->getWidth(), (float)_target->getHeight());
    shader.setUniform("u_isup", _texFil != NULL);

    _vbo->render( &shader );

    _target->unbind();
}