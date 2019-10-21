#include "polyline.h"

#include "polarPoint.h"
#include "tools/geom.h"

Polyline::Polyline():m_centroid(0.0,0.0,0.0),m_bChange(true){
    m_points.clear();
}

Polyline::Polyline(const Polyline &_poly):m_centroid(0.0,0.0,0.0),m_bChange(true){
    for (int i = 0; i < _poly.size(); i++) {
        add(_poly[i]);
    }
}

Polyline::Polyline(const std::vector<glm::vec3> & _points):m_centroid(0.0,0.0,0.0),m_bChange(true){
    add(_points);
}

Polyline::Polyline(const glslViewer::Rectangle &_rect, float _radiants){
    if (_radiants == 0) {
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

Polyline::~Polyline(){
   m_points.clear();
}

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
    for (unsigned int i = 0; i < _points.size(); i++) {
        add(_points[i]);
    }
}

void Polyline::add(const glm::vec3* verts, int numverts) {
    for (int i = 0 ; i < numverts ; i++) {
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
    for (unsigned int i = 0; i < m_points.size()-1; i++){
        PolarPoint polar(m_points[i],m_points[i+1]);
        if(distance+polar.r <= _dist){
            return  m_points[i] + PolarPoint(polar.a,_dist-distance).getXY();
        }
        distance += polar.r;
	}
    return glm::vec3(0.,0.,0.);
}

glslViewer::Rectangle Polyline::getBoundingBox() const {
    glslViewer::Rectangle box;
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
        for (unsigned int i = 0; i < m_points.size()-1; i++){
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
	for (int i=1; i<=N; i++) {
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
