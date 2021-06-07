#pragma once

#include "vbo.h"
#include "fbo.h"
#include "shader.h"

#include <functional>

// Reference:
// - Convolution Pyramids, Farbman et al., 2011 (https://www.cse.huji.ac.il/labs/cglab/projects/convpyr/data/convpyr-small.pdf)
// - Rendu (https://github.com/kosua20/Rendu)
// - ofxPoissonFill (https://github.com/LingDong-/ofxPoissonFill)
//

#define PYRAMID_MAX_LAYERS 12

class Pyramid {
public:
    Pyramid();
    virtual ~Pyramid();

    void    allocate(int _width, int _height);
    void    process(const Fbo *_fbo);

    bool    isAllocated() const {return m_depth != 0; }
    int     getDepth() const { return m_depth; }
    const Fbo* getResult(unsigned int index = 0) const;

    std::function<void(Fbo*,const Fbo*,const Fbo*, int)> pass;
private:
    Fbo     m_downs[PYRAMID_MAX_LAYERS];
    Fbo     m_ups[PYRAMID_MAX_LAYERS];

    int     m_depth;
};
