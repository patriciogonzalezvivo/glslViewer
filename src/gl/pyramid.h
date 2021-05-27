#pragma once

#include "vbo.h"
#include "fbo.h"
#include "shader.h"

// Reference:
// - Convolution Pyramids, Farbman et al., 2011
//      (https://www.cse.huji.ac.il/labs/cglab/projects/convpyr/data/convpyr-small.pdf)
// - Rendu (https://github.com/kosua20/Rendu)
// - ofxPoissonFill (https://github.com/LingDong-/ofxPoissonFill)
//

#define PYRAMID_MAX_LAYERS 12

class Pyramid {
public:
    Pyramid();
    virtual ~Pyramid();

    void    allocate(int _width, int _height);
    void    process(Vbo* _vbo, const Fbo *_fbo);

    bool    isAllocated() const {return m_depth != 0; }
    const Fbo* getResult() const { return &m_ups[m_depth-1]; }

    Shader  shader;
private:
    void    pass(Vbo* _vbo, Fbo *_fbo, const Fbo *_texUnf, const Fbo *_texFil);

    Fbo     m_downs[PYRAMID_MAX_LAYERS];
    Fbo     m_ups[PYRAMID_MAX_LAYERS];

    int     m_width;
    int     m_height;
    int     m_depth;
};
