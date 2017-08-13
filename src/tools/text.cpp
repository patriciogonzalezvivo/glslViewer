#include "tools/text.h"

#include <algorithm>

void toLower(std::string &_string) {
    for (int i = 0; _string[i]; i++) {
        _string[i] = tolower(_string[i]);
    }
}

std::string getLower(const std::string& _string) {
    std::string std = _string;
    toLower(std);
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

int getInt(const std::string &_string) {
    int x = 0;
    std::istringstream cur(_string);
    cur >> x;
    return x;
}

float getFloat(const std::string &_string) {
    float x = 0;
    std::istringstream cur(_string);
    cur >> x;
    return x;
}

double getDouble(const std::string &_string) {
    double x = 0;
    std::istringstream cur(_string);
    cur >> x;
    return x;
}

bool getBool(const std::string &_string) {
    static const std::string trueString = "true";
    static const std::string falseString = "false";

    std::string lower = getLower(_string);

    if (lower == trueString) {
        return true;
    }
    if (lower == falseString) {
        return false;
    }

    bool x = false;
    std::istringstream cur(lower);
    cur >> x;
    return x;
}

char getChar(const std::string &_string) {
    char x = '\0';
    std::istringstream cur(_string);
    cur >> x;
    return x;
}

std::vector<std::string> split(const std::string &_string, char sep) {
    std::vector<std::string> tokens;
    std::size_t start = 0, end = 0;
    while ((end = _string.find(sep, start)) != std::string::npos) {
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
