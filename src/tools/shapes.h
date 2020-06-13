#pragma once

#include "glm/glm.hpp"
#include "../types/mesh.h"

Mesh line(const glm::vec3 &_a, const glm::vec3 &_b);

Mesh lineTo(const glm::vec3 &_a, const glm::vec3 &_dir, float _size);

Mesh cross(const glm::vec3 &_pos, float _width);

Mesh rect(float _x, float _y, float _w, float _h);

Mesh axis(float _size, float _y = 0.0);

Mesh grid(float _size, int _segments, float _y = 0.0);
Mesh grid(float _width, float _height, int _columns, int _rows, float _y = 0.0);

Mesh floor(float _area, int _subD, float _y = 0.0);

Mesh cube(float _size);
Mesh cubeCorners(const glm::vec3 &_min_v, const glm::vec3 &_max_v, float _size);
Mesh cubeCorners(const std::vector<glm::vec3> &_pts, float _size = 1.0);
