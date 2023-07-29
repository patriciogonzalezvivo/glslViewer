#pragma once

#include <string>
#include "glm/glm.hpp"

// Search for one apearance
bool findId(const std::string& program, const char* id);

// -1.0 means it have a fixed size
float getBufferSize(const std::string& _source, const std::string& _name, glm::vec2& _size);

int  countBuffers(const std::string& _source);
int  countDoubleBuffers(const std::string& _source);
int  countConvolutionPyramid(const std::string& _source);

bool checkConvolutionPyramid(const std::string& _source);

bool checkFloor(const std::string& _source);
bool checkBackground(const std::string& _source);
bool checkPostprocessing(const std::string& _source);

bool checkPositionBuffer(const std::string& _source);
bool checkNormalBuffer(const std::string& _source);
int  countSceneBuffers(const std::string& _source);