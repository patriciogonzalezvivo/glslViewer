#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <vector>

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
