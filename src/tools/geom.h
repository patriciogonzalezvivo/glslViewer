#pragma once

#include <vector>

#include "glm/glm.hpp"

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
int signValue(float _n);

void wrapRad(double &_rad);
void wrapDeg(float &_deg);

void scale(glm::vec3 &_vec, float _length);
glm::vec3 getScaled(const glm::vec3 &_vec, float _length);

float getArea(const std::vector<glm::vec3> &_pts);
glm::vec3 getCentroid(const std::vector<glm::vec3> &_pts);
void getBoundingBox(const std::vector<glm::vec3> &_pts, glm::vec3 &_min, glm::vec3 &_max);
float getSize(const std::vector<glm::vec3> &_pts);

void simplify(std::vector<glm::vec3> &_pts, float _tolerance=0.3f);
std::vector<glm::vec3> getSimplify(const std::vector<glm::vec3> &_pts, float _tolerance=0.3f);

std::vector<glm::vec3> getConvexHull(std::vector<glm::vec3> &_pts);
std::vector<glm::vec3> getConvexHull(const std::vector<glm::vec3> &_pts);

void getNormal(const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2, glm::vec3 &N);

//---------------------------------------- Conversions
float mapValue(const float &_value, const float &_inputMin, const float &_inputMax, const float &_outputMin, const float &_outputMax, bool _clamp = true );
