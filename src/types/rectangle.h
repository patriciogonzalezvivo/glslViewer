/*
Copyright (c) 2014, Patricio Gonzalez Vivo
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <vector>

#include "glm/glm.hpp"

class Rectangle {
public:
    
    Rectangle();
    Rectangle(const glm::vec4 &_vec4);
    Rectangle(const glm::ivec4 &_viewPort);
    Rectangle(const Rectangle &_rectangel, const float &_margin = 0.0);
    Rectangle(const float &_x, const float &_y, const float &_width, const float &_height);
    virtual ~Rectangle();
    
    void    set(const glm::vec4 &_vec4);
    void    set(const glm::ivec4 &_viewPort);
    void    set(const float &_x, const float &_y, const float &_width, const float &_height);
    
    void    translate(const glm::vec3 &_pos);
    
    void    growToInclude(const glm::vec3& _point);
    void    growToInclude(const std::vector<glm::vec3> &_points);
    
    bool    inside(const float &_px, const float &_py) const;
    bool    inside(const glm::vec3& _p) const;
    bool    inside(const Rectangle& _rect) const;
    bool    inside(const glm::vec3& p0, const glm::vec3& p1) const;
    
    bool    intersects(const Rectangle& _rect) const;
    
    float   getMinX() const;
    float   getMaxX() const;
    float   getMinY() const;
    float   getMaxY() const;
    
    float   getLeft()   const;
    float   getRight()  const;
    float   getTop()    const;
    float   getBottom() const;
    
    glm::vec3   getMin() const;
    glm::vec3   getMax() const;
    
    glm::vec3   getCenter() const;
    glm::vec3   getTopLeft() const;
    glm::vec3   getTopRight() const;
    glm::vec3   getBottomLeft() const;
    glm::vec3   getBottomRight() const;
    
    float   x,y,width,height;
};