#pragma once

#include "glm/glm.hpp"
#include "mesh.h"
#include "rectangle.h"

Mesh line (const glm::vec3 &_a, const glm::vec3 &_b);
void drawLine(const glm::vec3 &_a, const glm::vec3 &_b);

Mesh cross(const glm::vec3 &_pos, float _width);
void drawCross(const glm::vec3 &_pos, float _width = 3.5);

Mesh crossCube(const std::vector<glm::vec3> &_pts);
Mesh cube(float _size);

Mesh rect (float _x, float _y, float _w, float _h);
Mesh rect (const Rectangle &_rect);

Mesh rectBorders(const Rectangle &_rect);
void drawBorders( const Rectangle &_rect );

Mesh rectCorners(const Rectangle &_rect, float _width = 4.);
void drawCorners(const Rectangle &_rect, float _width = 4.);

Mesh polyline (const std::vector<glm::vec3> &_pts );
void drawPolyline(const std::vector<glm::vec3> &_pts );
