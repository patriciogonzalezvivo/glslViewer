#pragma once

#include "glm/glm.hpp"
#include "types/mesh.h"
#include "types/rectangle.h"

Mesh rect (float _x, float _y, float _w, float _h);
Mesh rect (const Rectangle &_rect);

Mesh rectBorders(const Rectangle &_rect);
Mesh rectCorners(const Rectangle &_rect, float _width = 4.);

Mesh cross(const glm::vec3 &_pos, float _width);
void drawCross(const glm::vec3 &_pos, const float &_width = 3.5);

Mesh line (const glm::vec3 &_a, const glm::vec3 &_b);
Mesh polyline (const std::vector<glm::vec3> &_pts );
