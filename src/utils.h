#pragma once

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
        (*into) += buffer + "\n";
    }

    file.close();
    return true;
}
