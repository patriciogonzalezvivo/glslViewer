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
