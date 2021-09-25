#pragma once

#include "ada/gl/vbo.h"
#include "ada/gl/fbo.h"
#include "ada/gl/shader.h"

#include <functional>

// Reference:
// - Convolution Pyramids, Farbman et al., 2011 (https://www.cse.huji.ac.il/labs/cglab/projects/convpyr/data/convpyr-small.pdf)
// - Rendu (https://github.com/kosua20/Rendu)
// - ofxPoissonFill (https://github.com/LingDong-/ofxPoissonFill)
//

#define CONVOLUTION_PYRAMID_MAX_LAYERS 12

class ConvolutionPyramid {
public:
    ConvolutionPyramid();
    virtual ~ConvolutionPyramid();

    void    allocate(int _width, int _height);
    void    process(const ada::Fbo *_fbo);

    bool            isAllocated() const {return m_depth != 0; }
    unsigned int    getDepth() const { return m_depth; }
    const ada::Fbo* getResult(unsigned int index = 0) const;

    std::function<void(ada::Fbo*,const ada::Fbo*,const ada::Fbo*, int)> pass;
private:
    ada::Fbo    m_downs[CONVOLUTION_PYRAMID_MAX_LAYERS];
    ada::Fbo    m_ups[CONVOLUTION_PYRAMID_MAX_LAYERS];

    unsigned int m_depth;
};
