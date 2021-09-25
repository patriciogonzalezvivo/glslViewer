#pragma once

#include <vector>

#include "glm/glm.hpp"
#include "ada/gl/mesh.h"

#ifndef PI
#define PI       3.14159265358979323846
#endif

#ifndef TWO_PI
#define TWO_PI   6.28318530717958647693
#endif

#ifndef FOUR_PI
#define FOUR_PI 12.56637061435917295385
#endif

#ifndef HALF_PI
#define HALF_PI  1.57079632679489661923
#endif

#ifndef QUARTER_PI
#define QUARTER_PI 0.785398163
#endif

#ifndef FLT_EPSILON
#define FLT_EPSILON 1.19209290E-07F
#endif

#ifndef MIN
#define MIN(x,y) (((x) < (y)) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#endif

#ifndef CLAMP
#define CLAMP(val,min,max) ((val) < (min) ? (min) : ((val > max) ? (max) : (val)))
#endif

#ifndef ABS
#define ABS(x) (((x) < 0) ? -(x) : (x))
#endif

//---------------------------------------- Geom

float getArea(const std::vector<glm::vec3>& _pts);
glm::vec3 getCentroid(const std::vector<glm::vec3>& _pts);
void getBoundingBox(const std::vector<glm::vec3>& _pts, glm::vec3& _min, glm::vec3& _max);
void expandBoundingBox(const std::vector<glm::vec3>& _pts, glm::vec3& _min, glm::vec3& _max);
void expandBoundingBox(const glm::vec3& _pt, glm::vec3& _min, glm::vec3& _max);

void calcNormal(const glm::vec3& _v0, const glm::vec3& _v1, const glm::vec3& _v2, glm::vec3& _N);

ada::Mesh line(const glm::vec3 &_a, const glm::vec3 &_b);

ada::Mesh lineTo(const glm::vec3 &_a, const glm::vec3 &_dir, float _size);

ada::Mesh cross(const glm::vec3 &_pos, float _width);

ada::Mesh rect(float _x, float _y, float _w, float _h);

ada::Mesh axis(float _size, float _y = 0.0);

ada::Mesh grid(float _size, int _segments, float _y = 0.0);
ada::Mesh grid(float _width, float _height, int _columns, int _rows, float _y = 0.0);

ada::Mesh floor(float _area, int _subD, float _y = 0.0);

ada::Mesh cube(float _size);
ada::Mesh cubeCorners(const glm::vec3 &_min_v, const glm::vec3 &_max_v, float _size);
ada::Mesh cubeCorners(const std::vector<glm::vec3> &_pts, float _size = 1.0);
