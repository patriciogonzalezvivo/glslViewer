#include "tools/text.h"

#include <algorithm>

std::string getLower(const std::string& _string) {
    std::string std = _string;
    for (int i = 0; _string[i]; i++) {
        std[i] = tolower(_string[i]);
    }
    return std;
}

void purifyString(std::string& _string) {
    for (std::string::iterator it = _string.begin(), itEnd = _string.end(); it!=itEnd; ++it) {
        if (static_cast<uint>(*it) < 32 || static_cast<uint>(*it) > 127) {
            (*it) = ' ';
        }
    }
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

    std::string lower = getLower(_string);

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
    strStream<< _vec.x << _sep << _vec.y << _sep;
    return strStream.str();
}

std::string toString(const glm::vec3 &_vec, char _sep) {
    std::ostringstream strStream;
    strStream<< _vec.x << _sep << _vec.y << _sep << _vec.z;
    return strStream.str();
}

std::string toString(const glm::vec4 &_vec, char _sep) {
    std::ostringstream strStream;
    strStream<< _vec.x << _sep << _vec.y << _sep << _vec.z << _sep << _vec.w;
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

std::vector<std::string> split(const std::string &_string, char _sep) {
    std::vector<std::string> tokens;
    std::size_t start = 0, end = 0;
    while ((end = _string.find(_sep, start)) != std::string::npos) {
        if (end != start) {
          tokens.push_back(_string.substr(start, end - start));
        }
        start = end + 1;
    }
    if (end != start) {
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
