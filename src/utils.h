#pragma once

#include <vector>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <cctype>
#include <iomanip>
#include <algorithm>


//---------------------------------------- Conversions

/*  Transform the string into lower letters */
inline void toLower( std::string &_str ){
    for (int i = 0; _str[i]; i++) {
        _str[i] = tolower(_str[i]);
    }
}

/*  Return new string with all into lower letters */
inline std::string getLower(const std::string& _str ){
    std::string std = _str;
    toLower(std);
    return std;
}

/*  Extract extrange characters from a string */
inline void purifyString( std::string& _s ){
    for ( std::string::iterator it = _s.begin(), itEnd = _s.end(); it!=itEnd; ++it){
        if ( static_cast<unsigned int>(*it) < 32 || static_cast<unsigned int>(*it) > 127 ){
            (*it) = ' ';
        }
    }
}

inline int getInt(const std::string &_intString) {
    int x = 0;
    std::istringstream cur(_intString);
    cur >> x;
    return x;
}

inline float getFloat(const std::string &_floatString) {
    float x = 0;
    std::istringstream cur(_floatString);
    cur >> x;
    return x;
}

inline double getDouble(const std::string &_doubleString) {
    double x = 0;
    std::istringstream cur(_doubleString);
    cur >> x;
    return x;
}

inline bool getBool(const std::string &_boolString) {
    static const std::string trueString = "true";
    static const std::string falseString = "false";
    
    std::string lower = getLower(_boolString);
    
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

inline char getChar(const std::string &_charString) {
    char x = '\0';
    std::istringstream cur(_charString);
    cur >> x;
    return x;
}

inline std::string getString(bool _bool){
    std::ostringstream strStream;
    strStream << (_bool?"true":"false") ;
    return strStream.str();
}

template <class T>
std::string getString(const T& _value){
    std::ostringstream out;
    out << _value;
    return out.str();
}

/// like sprintf "%4f" format, in this example precision=4
template <class T>
std::string getString(const T& _value, int _precision){
    std::ostringstream out;
    out << std::fixed << std::setprecision(_precision) << _value;
    return out.str();
}

/// like sprintf "% 4d" or "% 4f" format, in this example width=4, fill=' '
template <class T>
std::string getString(const T& _value, int _width, char _fill ){
    std::ostringstream out;
    out << std::fixed << std::setfill(_fill) << std::setw(_width) << _value;
    return out.str();
}

/// like sprintf "%04.2d" or "%04.2f" format, in this example precision=2, width=4, fill='0'
template <class T>
std::string getString(const T& _value, int _precision, int _width, char _fill ){
    std::ostringstream out;
    out << std::fixed << std::setfill(_fill) << std::setw(_width) << std::setprecision(_precision) << _value;
    return out.str();
}

//----------------------------------------  String operations

static std::string getLineNumber(const std::string& _source, unsigned _lineNumber){
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

/*  Return a vector of string from a _source string splits it using a delimiter */
static std::vector<std::string> splitString(const std::string& _source, const std::string& _delimiter = "", bool _ignoreEmpty = false) {
    std::vector<std::string> result;
    if (_delimiter.empty()) {
        result.push_back(_source);
        return result;
    }
    std::string::const_iterator substart = _source.begin(), subend;
    while (true) {
        subend = search(substart, _source.end(), _delimiter.begin(), _delimiter.end());
        std::string sub(substart, subend);
        
        if (!_ignoreEmpty || !sub.empty()) {
            result.push_back(sub);
        }
        if (subend == _source.end()) {
            break;
        }
        substart = subend + _delimiter.size();
    }
    return result;
}

//----------------------------------------  String I/O

static bool loadFromPath(const std::string& path, std::string* into) {
    std::ifstream file;
    std::string buffer;

    file.open(path.c_str());
    if(!file.is_open()) return false;
    while(!file.eof()) {
        getline(file, buffer);
	if(buffer.find("#include ") == 0){
		unsigned begin = buffer.find_first_of("\"");
		unsigned end = buffer.find_last_of("\"");
		if(begin != end){
			std::string file = buffer.substr(begin+1,end-begin-1);
			std::string newBuffer;
			if(loadFromPath(file,&newBuffer)){
				(*into) += "\n" + newBuffer + "\n";
			} else {
				std::cout << file << " not found" << std::endl;
			}
		}
	} else {
        	(*into) += buffer + "\n";
	}
    }

    file.close();
    return true;
}
