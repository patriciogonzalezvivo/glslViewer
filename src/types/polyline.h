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

#include "rectangle.h"

class Polyline {
public:
    
    Polyline();
    Polyline(const Polyline &_poly);
    Polyline(const std::vector<glm::vec3> &_points);
    Polyline(const Rectangle &_rect, float _radiants = 0);
//    virtual ~Polyline();
    
    virtual void add(const glm::vec3 &_point);
    void    add(const std::vector<glm::vec3> &_points);
	void    add(const glm::vec3* verts, int numverts);
    
    virtual glm::vec3 & operator [](const int &_index);
    virtual const glm::vec3 & operator [](const int &_index) const;
    
    virtual float       getArea();
    virtual glm::vec3   getCentroid();
    virtual const std::vector<glm::vec3> & getVertices() const;
    virtual glm::vec3   getPositionAt(const float &_dist) const;
    virtual Polyline get2DConvexHull();
	Rectangle        getBoundingBox() const;
    
    bool    isInside(float _x, float _y);
    
    virtual int size() const;

    std::vector<Polyline> splitAt(float _dist);
    
    virtual void    clear();
    virtual void    simplify(float _tolerance=0.3f);
    
protected:
    std::vector<glm::vec3>  m_points;
    glm::vec3   m_centroid;
    
    bool        m_bChange;
};
