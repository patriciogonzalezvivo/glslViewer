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

#include "polyline.h"

#include "polarPoint.h"
#include "utils.h"

Polyline::Polyline():m_centroid(0.0,0.0,0.0),m_bChange(true){
    m_points.clear();
}

Polyline::Polyline(const Polyline &_poly):m_centroid(0.0,0.0,0.0),m_bChange(true){
    for (uint i = 0; i < _poly.size(); i++) {
        add(_poly[i]);
    }
}

Polyline::Polyline(const std::vector<glm::vec3> & _points):m_centroid(0.0,0.0,0.0),m_bChange(true){
    add(_points);
}

Polyline::Polyline(const Rectangle &_rect, float _radiants){
    if(_radiants == 0){
        add(_rect.getTopLeft());
        add(_rect.getTopRight());
        add(_rect.getBottomRight());
        add(_rect.getBottomLeft());
    } else {
        
        PolarPoint toTR = PolarPoint(_rect.getCenter(),_rect.getTopRight());
        PolarPoint toBR = PolarPoint(_rect.getCenter(),_rect.getBottomRight());
        
        toTR.a += _radiants;
        toBR.a += _radiants;
        
        add(toTR.getXY());
        add(toBR.getXY());
        
        toTR.a += PI;
        toBR.a += PI;

        add(toTR.getXY());
        add(toBR.getXY());
    }
}

//Polyline::~Polyline(){
//    m_points.clear();
//}

int Polyline::size() const {
    return m_points.size();
}

void Polyline::clear(){
    m_points.clear();
}

void Polyline::add( const glm::vec3 & _point ){
    m_points.push_back(_point);
    m_bChange = true;
}

void Polyline::add(const std::vector<glm::vec3> & _points){
    for (uint i = 0; i < _points.size(); i++) {
        add(_points[i]);
    }
}

void Polyline::add(const glm::vec3* verts, int numverts) {
    for (uint i = 0 ; i < numverts ; i++) {
        add(verts[i]);
    }
}

glm::vec3 & Polyline::operator [](const int &_index){
    return m_points[_index];
}

const glm::vec3 & Polyline::operator [](const int &_index) const {
    return m_points[_index];
}

void Polyline::simplify(float _tolerance){
    ::simplify(m_points,_tolerance);
}

const std::vector<glm::vec3> & Polyline::getVertices() const{
	return m_points;
}

glm::vec3 Polyline::getPositionAt(const float &_dist) const{
    float distance = 0.0;
    for (uint i = 0; i < m_points.size()-1; i++){
        PolarPoint polar(m_points[i],m_points[i+1]);
        if(distance+polar.r <= _dist){
            return  m_points[i] + PolarPoint(polar.a,_dist-distance).getXY();
        }
        distance += polar.r;
	}
    return glm::vec3(0.,0.,0.);
}

Rectangle Polyline::getBoundingBox() const {
	Rectangle box;
    box.growToInclude(m_points);
	return box;
}

glm::vec3 Polyline::getCentroid() {
    if(m_bChange){
        m_centroid = ::getCentroid(m_points);
        m_bChange = false;
    }
    return m_centroid;
}

float Polyline::getArea(){
    return ::getArea(m_points);
}

std::vector<Polyline> Polyline::splitAt(float _dist){
    std::vector<Polyline> RTA;
    if (size()>0) {
        Polyline buffer;
        
        buffer.add(m_points[0]);
        float distance = 0.0;
        for (uint i = 0; i < m_points.size()-1; i++){
            PolarPoint polar(m_points[i],m_points[i+1]);
            if(distance+polar.r <= _dist){
                buffer.add(m_points[i] + PolarPoint(polar.a,_dist-distance).getXY());
                RTA.push_back(buffer);
                buffer.clear();
                buffer.add(m_points[i] + PolarPoint(polar.a,_dist-distance).getXY());
                break;
            }
            buffer.add(m_points[i+1]);
            distance += polar.r;
        }
        RTA.push_back(buffer);
    }
    return RTA;
}

// http://www.geeksforgeeks.org/how-to-check-if-a-given-point-lies-inside-a-polygon/
//
bool Polyline::isInside(float _x, float _y){
	int counter = 0;
	double xinters;
    glm::vec3 p1,p2;
    
	int N = size();
    
	p1 = m_points[0];
	for (uint i=1;i<=N;i++) {
		p2 = m_points[i % N];
		if (_y > MIN(p1.y,p2.y)) {
            if (_y <= MAX(p1.y,p2.y)) {
                if (_x <= MAX(p1.x,p2.x)) {
                    if (p1.y != p2.y) {
                        xinters = (_y-p1.y)*(p2.x-p1.x)/(p2.y-p1.y)+p1.x;
                        if (p1.x == p2.x || _x <= xinters)
                            counter++;
                    }
                }
            }
		}
		p1 = p2;
	}
    
	if (counter % 2 == 0) return false;
	else return true;
}

//  http://geomalgorithms.com/a12-_hull-3.html
//
Polyline Polyline::get2DConvexHull(){
    return Polyline( getConvexHull(m_points) );
}