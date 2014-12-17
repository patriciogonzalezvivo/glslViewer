#pragma once

#include <algorithm>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>

static std::string getExtention(const std::string _path){
    return _path.substr(_path.find_last_of('.'));
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
