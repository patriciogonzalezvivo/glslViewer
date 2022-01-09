#pragma once

#include <string>
#include "glm/glm.hpp"

// Search for one apearance
bool findId(const std::string& program, const char* id);

bool getBufferSize(const std::string& _source, size_t _index, glm::vec2& _size);
int  countBuffers(const std::string& _source);
int  countConvolutionPyramid(const std::string& _source);

bool checkConvolutionPyramid(const std::string& _source);
bool checkFloor(const std::string& _source);
bool checkBackground(const std::string& _source);

bool checkPostprocessing(const std::string& _source);
bool checkPattern(const std::string& _str);

std::string getUniformName(const std::string& _str);