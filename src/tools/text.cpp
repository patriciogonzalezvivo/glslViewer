#include "tools/text.h"

#include <algorithm>
#include <regex>

std::string toLower(const std::string& _string) {
    std::string std = _string;
    for (int i = 0; _string[i]; i++) {
        std[i] = tolower(_string[i]);
    }
    return std;
}

std::string toUpper(const std::string& _string) {
    std::string std = _string;
    for (int i = 0; _string[i]; i++) {
        std[i] = toupper(_string[i]);
    }
    return std;
}

std::string toUnderscore(const std::string& _string){
    std::string std = _string;
    std::replace(std.begin(), std.end(), ' ', '_');
    return std;
}

std::string purifyString(const std::string& _string) {
    std::string std = _string;
    for (std::string::iterator it = std.begin(), itEnd = std.end(); it!=itEnd; ++it) {
        if (static_cast<uint>(*it) < 32 || 
            static_cast<uint>(*it) > 127 || 
            *it == '.' ||
            *it == '-' ||
            *it == '\\'||
            *it == '/' ) {
            (*it) = '_';
        }
    }
    return std;
}

bool isDigit(const std::string &_string) {
  return _string.find_first_not_of( "0123456789" ) == std::string::npos;
}

bool isFloat(const std::string &_string) {
    std::istringstream iss(_string);
    float dummy;
    iss >> std::skipws >> dummy;
    // if it's a number
    if (iss && iss.eof()) {
        std::string::const_iterator it = _string.begin();
        while (it != _string.end()) {
            if (*it == '.') {
                // That contain a .
                return true;
            }
            ++it;
        }
    }
    return false;
}

//---------------------------------------- Conversions
int toInt(const std::string &_string) {
    int x = 0;
    std::istringstream cur(_string);
    cur >> x;
    return x;
}

float toFloat(const std::string &_string) {
    float x = 0;
    std::istringstream cur(_string);
    cur >> x;
    return x;
}

double toDouble(const std::string &_string) {
    double x = 0;
    std::istringstream cur(_string);
    cur >> x;
    return x;
}

bool toBool(const std::string &_string) {
    static const std::string trueString = "true";
    static const std::string falseString = "false";

    std::string lower = toLower(_string);

    if(lower == trueString) {
        return true;
    }
    if(lower == falseString) {
        return false;
    }

    bool x = false;
    std::istringstream cur(lower);
    cur >> x;
    return x;
}

char toChar(const std::string &_string) {
    char x = '\0';
    std::istringstream cur(_string);
    cur >> x;
    return x;
}

std::string toString(bool _bool) {
    std::ostringstream strStream;
    strStream << (_bool?"true":"false") ;
    return strStream.str();
}

std::string toString(const glm::vec2 &_vec, char _sep) {
    std::ostringstream strStream;
    strStream << std::fixed << std::setprecision(3) << _vec.x << _sep;
    strStream << std::fixed << std::setprecision(3) << _vec.y << _sep;
    return strStream.str();
}

std::string toString(const glm::vec3 &_vec, char _sep) {
    std::ostringstream strStream;
    strStream << std::fixed << std::setprecision(3) << _vec.x << _sep;
    strStream << std::fixed << std::setprecision(3) << _vec.y << _sep; 
    strStream << std::fixed << std::setprecision(3) << _vec.z;
    return strStream.str();
}

std::string toString(const glm::vec4 &_vec, char _sep) {
    std::ostringstream strStream;
    strStream << std::fixed << std::setprecision(3) << _vec.x << _sep;
    strStream << std::fixed << std::setprecision(3) << _vec.y << _sep;
    strStream << std::fixed << std::setprecision(3) << _vec.z << _sep; 
    strStream << std::fixed << std::setprecision(3) << _vec.w;
    return strStream.str();
}

std::ostream& operator<<(std::ostream& os, const glm::vec3& vec) {
    os << vec.x << ", " << vec.y << ", " << vec.z; 
    return os;
}

std::istream& operator>>(std::istream& is, glm::vec3& vec) {
    is >> vec.x;
    is.ignore(2);
    is >> vec.y;
    is.ignore(2);
    is >> vec.z;
    return is;
}

std::vector<std::string> split(const std::string &_string, char _sep, bool _tolerate_empty) {
    std::vector<std::string> tokens;
    std::size_t start = 0, end = 0;
    while ((end = _string.find(_sep, start)) != std::string::npos) {
        if (end != start || _tolerate_empty) {
          tokens.push_back(_string.substr(start, end - start));
        }
        start = end + 1;
    }
    if (end != start || _tolerate_empty) {
       tokens.push_back(_string.substr(start));
    }
    return tokens;
}

bool beginsWith(const std::string &_stringA, const std::string &_stringB) {
    for (uint i = 0; i < _stringB.size(); i++) {
        if (_stringB[i] != _stringA[i]) {
            return false;
        }
    }
    return true;
}

std::string getLineNumber(const std::string& _source, unsigned _lineNumber) {
    std::string delimiter = "\n";
    std::string::const_iterator substart = _source.begin(), subend;

    unsigned index = 1;
    while (true) {
        subend = search(substart, _source.end(), delimiter.begin(), delimiter.end());
        std::string sub(substart, subend);

        if (index == _lineNumber) {
            return sub;
        }
        index++;

        if (subend == _source.end()) {
            break;
        }

        substart = subend + delimiter.size();
    }

    return "NOT FOUND";
}

// Quickly determine if a shader program contains the specified identifier.
bool find_id(const std::string& program, const char* id) {
    return std::strstr(program.c_str(), id) != 0;
}

// Count how many BUFFERS are in the shader
int count_buffers(const std::string &_source) {
    // Split Source code in lines
    std::vector<std::string> lines = split(_source, '\n');

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

// Count how many BUFFERS are in the shader
bool check_for_background(const std::string &_source) {
    // Split Source code in lines
    std::vector<std::string> lines = split(_source, '\n');

    std::regex re(R"((?:^\s*#if|^\s*#elif)(?:\s+)(defined\s*\(\s*BACKGROUND)(?:\s*\))|(?:^\s*#ifdef\s+BACKGROUND)|(?:^\s*#ifndef\s+BACKGROUND))");
    std::smatch match;

    for (unsigned int l = 0; l < lines.size(); l++) {
        if (std::regex_search(lines[l], match, re)) {
            return true;
        }
    }

    return false;
}

// Count how many BUFFERS are in the shader
bool check_for_floor(const std::string &_source) {
    // Split Source code in lines
    std::vector<std::string> lines = split(_source, '\n');

    std::regex re(R"((?:^\s*#if|^\s*#elif)(?:\s+)(defined\s*\(\s*FLOOR)(?:\s*\))|(?:^\s*#ifdef\s+FLOOR)|(?:^\s*#ifndef\s+FLOOR))");
    std::smatch match;

    for (unsigned int l = 0; l < lines.size(); l++) {
        if (std::regex_search(lines[l], match, re)) {
            return true;
        }
    }

    return false;
}

bool check_for_postprocessing(const std::string &_source) {
    // Split Source code in lines
    std::vector<std::string> lines = split(_source, '\n');

    std::regex re(R"((?:^\s*#if|^\s*#elif)(?:\s+)(defined\s*\(\s*POSTPROCESSING)(?:\s*\))|(?:^\s*#ifdef\s+POSTPROCESSING)|(?:^\s*#ifndef\s+POSTPROCESSING))");
    std::smatch match;

    for (unsigned int l = 0; l < lines.size(); l++) {
        if (std::regex_search(lines[l], match, re)) {
            return true;
        }
    }

    return false;
}

