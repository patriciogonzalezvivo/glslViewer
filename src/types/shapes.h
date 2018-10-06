#pragma once

#include "glm/glm.hpp"
#include "mesh.h"
#include "rectangle.h"

Mesh line(const glm::vec3 &_a, const glm::vec3 &_b);

Mesh lineTo(const glm::vec3 &_a, const glm::vec3 &_dir, float _size);

Mesh cross(const glm::vec3 &_pos, float _width);

Mesh rect(float _x, float _y, float _w, float _h);
Mesh rect(const Rectangle &_rect);

Mesh rectBorders(const Rectangle &_rect);
Mesh rectCorners(const Rectangle &_rect, float _width = 4.);

Mesh polyline(const std::vector<glm::vec3> &_pts );

Mesh axis(float _size);

Mesh grid(float _size, int _segments);
Mesh grid(float _width, float _height, int _columns, int _rows);

Mesh cube(float _size);
Mesh cubeCorners(const glm::vec3 &_min_v, const glm::vec3 &_max_v, float _size);
Mesh cubeCorners(const std::vector<glm::vec3> &_pts, float _size = 1.0);