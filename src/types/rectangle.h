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
