#pragma once

#include <string>
#include "glm/glm.hpp"

// Search for one apearance
bool findId(const std::string& program, const char* id);

bool getBufferSize(const std::string& _source, const std::string& _name, glm::vec2& _size);

int  countBuffers(const std::string& _source);
int  countDoubleBuffers(const std::string& _source);
int  countConvolutionPyramid(const std::string& _source);

bool checkConvolutionPyramid(const std::string& _source);

bool checkFloor(const std::string& _source);
bool checkBackground(const std::string& _source);

bool checkPostprocessing(const std::string& _source);
bool checkPattern(const std::string& _str);

std::string getUniformName(const std::string& _str);