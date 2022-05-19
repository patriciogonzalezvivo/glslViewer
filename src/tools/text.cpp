#include "text.h"

#include <algorithm>
#include <regex>
#include <cstring>
#include "ada/tools/text.h"

// Quickly determine if a shader program contains the specified identifier.
bool findId(const std::string& program, const char* id) {
    return std::strstr(program.c_str(), id) != 0;
}


// Count how many BUFFERS are in the shader
int countBuffers(const std::string& _source) {
    // Split Source code in lines
    std::vector<std::string> lines = ada::split(_source, '\n');

    // Group results in a vector to check for duplicates
    std::vector<std::string> results;

    // Regext to search for #ifdef BUFFER_[NUMBER], #if defined( BUFFER_[NUMBER] ) and #elif defined( BUFFER_[NUMBER] ) occurences
    std::regex re(R"((?:^\s*#if|^\s*#elif)(?:\s+)(defined\s*\(\s*BUFFER_)(\d+)(?:\s*\))|(?:^\s*#ifdef\s+BUFFER_)(\d+))");
    std::smatch match;

    // For each line search for
    for (unsigned int l = 0; l < lines.size(); l++) {

        // if there are matches
        if (std::regex_search(lines[l], match, re)) {
            // Depending the case can be in the 2nd or 3rd group
            std::string number = std::ssub_match(match[2]).str();
            if (number.size() == 0) {
                number = std::ssub_match(match[3]).str();
            }

            // Check if it's already defined
            bool already = false;
            for (unsigned int i = 0; i < results.size(); i++) {
                if (results[i] == number) {
                    already = true;
                    break;
                }
            }

            // If it's not add it
            if (!already) {
                results.push_back(number);
            }
        }
    }

    // return the number of results
    return results.size();
}

bool getBufferSize(const std::string& _source, size_t _index, glm::vec2& _size) {
    std::regex re(R"(uniform\s*sampler2D\s*u_buffer(\d+)\;\s*\/\/*\s(\d+)x(\d+))");
    std::smatch match;

    // Split Source code in lines
    std::vector<std::string> lines = ada::split(_source, '\n');
    for (unsigned int l = 0; l < lines.size(); l++) {
        if (std::regex_search(lines[l], match, re)) {
            if (match[1] == ada::toString(_index)) {
                _size.x = ada::toFloat(match[2]);
                _size.y = ada::toFloat(match[3]);
                return true;
            }
        }
    }

    return false;
}


// Count how many BUFFERS are in the shader
int countDoubleBuffers(const std::string& _source) {
    // Split Source code in lines
    std::vector<std::string> lines = ada::split(_source, '\n');

    // Group results in a vector to check for duplicates
    std::vector<std::string> results;

    // Regext to search for #ifdef BUFFER_[NUMBER], #if defined( BUFFER_[NUMBER] ) and #elif defined( BUFFER_[NUMBER] ) occurences
    std::regex re(R"((?:^\s*#if|^\s*#elif)(?:\s+)(defined\s*\(\s*DOUBLE_BUFFER_)(\d+)(?:\s*\))|(?:^\s*#ifdef\s+DOUBLE_BUFFER_)(\d+))");
    std::smatch match;

    // For each line search for
    for (unsigned int l = 0; l < lines.size(); l++) {

        // if there are matches
        if (std::regex_search(lines[l], match, re)) {
            // Depending the case can be in the 2nd or 3rd group
            std::string number = std::ssub_match(match[2]).str();
            if (number.size() == 0) {
                number = std::ssub_match(match[3]).str();
            }

            // Check if it's already defined
            bool already = false;
            for (unsigned int i = 0; i < results.size(); i++) {
                if (results[i] == number) {
                    already = true;
                    break;
                }
            }

            // If it's not add it
            if (!already) {
                results.push_back(number);
            }
        }
    }

    // return the number of results
    return results.size();
}

bool getDoubleBufferSize(const std::string& _source, size_t _index, glm::vec2& _size) {
    std::regex re(R"(uniform\s*sampler2D\s*u_doubleBuffer(\d+)\;\s*\/\/*\s(\d+)x(\d+))");
    std::smatch match;

    // Split Source code in lines
    std::vector<std::string> lines = ada::split(_source, '\n');
    for (unsigned int l = 0; l < lines.size(); l++) {
        if (std::regex_search(lines[l], match, re)) {
            if (match[1] == ada::toString(_index)) {
                _size.x = ada::toFloat(match[2]);
                _size.y = ada::toFloat(match[3]);
                return true;
            }
        }
    }

    return false;
}


// Count how many BUFFERS are in the shader
bool checkBackground(const std::string& _source) {
    std::regex re(R"((?:^\s*#if|^\s*#elif)(?:\s+)(defined\s*\(\s*BACKGROUND)(?:\s*\))|(?:^\s*#ifdef\s+BACKGROUND)|(?:^\s*#ifndef\s+BACKGROUND))");
    std::smatch match;

    // Split Source code in lines
    std::vector<std::string> lines = ada::split(_source, '\n');
    for (unsigned int l = 0; l < lines.size(); l++) {
        if (std::regex_search(lines[l], match, re)) {
            return true;
        }
    }

    return false;
}

// Count how many BUFFERS are in the shader
bool checkFloor(const std::string& _source) {
    // Split Source code in lines
    std::vector<std::string> lines = ada::split(_source, '\n');

    std::regex re(R"((?:^\s*#if|^\s*#elif)(?:\s+)(defined\s*\(\s*FLOOR)(?:\s*\))|(?:^\s*#ifdef\s+FLOOR)|(?:^\s*#ifndef\s+FLOOR))");
    std::smatch match;

    for (unsigned int l = 0; l < lines.size(); l++) {
        if (std::regex_search(lines[l], match, re)) {
            return true;
        }
    }

    return false;
}

bool checkPostprocessing(const std::string& _source) {
    // Split Source code in lines
    std::vector<std::string> lines = ada::split(_source, '\n');

    std::regex re(R"((?:^\s*#if|^\s*#elif)(?:\s+)(defined\s*\(\s*POSTPROCESSING)(?:\s*\))|(?:^\s*#ifdef\s+POSTPROCESSING)|(?:^\s*#ifndef\s+POSTPROCESSING))");
    std::smatch match;

    for (unsigned int l = 0; l < lines.size(); l++) {
        if (std::regex_search(lines[l], match, re)) {
            return true;
        }
    }

    return false;
}

// Count how many CONVOLUTION_PYRAMID_ are in the shader
int countConvolutionPyramid(const std::string& _source) {
    // Split Source code in lines
    std::vector<std::string> lines = ada::split(_source, '\n');

    // Group results in a vector to check for duplicates
    std::vector<std::string> results;

    // Regext to search for #ifdef BUFFER_[NUMBER], #if defined( BUFFER_[NUMBER] ) and #elif defined( BUFFER_[NUMBER] ) occurences
    std::regex re(R"((?:^\s*#if|^\s*#elif)(?:\s+)(defined\s*\(\s*CONVOLUTION_PYRAMID_)(\d+)(?:\s*\))|(?:^\s*#ifdef\s+CONVOLUTION_PYRAMID_)(\d+))");
    std::smatch match;

    // For each line search for
    for (unsigned int l = 0; l < lines.size(); l++) {

        // if there are matches
        if (std::regex_search(lines[l], match, re)) {
            // Depending the case can be in the 2nd or 3rd group
            std::string number = std::ssub_match(match[2]).str();
            if (number.size() == 0) {
                number = std::ssub_match(match[3]).str();
            }

            // Check if it's already defined
            bool already = false;
            for (unsigned int i = 0; i < results.size(); i++) {
                if (results[i] == number) {
                    already = true;
                    break;
                }
            }

            // If it's not add it
            if (!already) {
                results.push_back(number);
            }
        }
    }

    // return the number of results
    return results.size();
}

bool checkConvolutionPyramid(const std::string& _source) {
    // Split Source code in lines
    std::vector<std::string> lines = ada::split(_source, '\n');

    std::regex re(R"((?:^\s*#if|^\s*#elif)(?:\s+)(defined\s*\(\s*CONVOLUTION_PYRAMID_ALGORITHM)(?:\s*\))|(?:^\s*#ifdef\s+CONVOLUTION_PYRAMID_ALGORITHM)|(?:^\s*#ifndef\s+CONVOLUTION_PYRAMID_ALGORITHM))");
    std::smatch match;

    for (unsigned int l = 0; l < lines.size(); l++) {
        if (std::regex_search(lines[l], match, re)) {
            return true;
        }
    }

    return false;
}

std::string getUniformName(const std::string& _str) {
    std::vector<std::string> values = ada::split(_str, '.');
    return "u_" + ada::toLower( ada::toUnderscore( ada::purifyString( values[0] ) ) );
}

bool checkPattern(const std::string& _str) {
    return  (_str.find('*') != std::string::npos) ||
            (_str.find('?') != std::string::npos);
}
