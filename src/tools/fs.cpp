#include <iostream>
#include <fstream> 

#include <sys/stat.h>

#include "tools/fs.h"

std::string getAbsPath (const std::string& str) {
    std::string abs_path = realpath(str.c_str(), NULL);
    std::size_t found = abs_path.find_last_of("\\/");
    if (found){
        return abs_path.substr(0,found);
    }
    else {
        return "";
    }
}

bool urlExists(const std::string& name) {
  struct stat buffer;
  return (stat (name.c_str(), &buffer) == 0);
}

std::string urlResolve(const std::string& path, const std::string& pwd, const std::vector<std::string> include_folders) {
    std::string url = pwd+'/'+path;
    if (urlExists(url)) {
        return url;
    }
    else {
        for (uint i = 0; i < include_folders.size(); i++) {
            std::string new_path = include_folders[i] + "/" + path;
            if (urlExists(new_path)) {
                return new_path;
            }
        }
        return path;
    }
}

bool loadFromPath(const std::string& path, std::string* into, const std::vector<std::string> include_folders) {
    std::ifstream file;
    std::string buffer;

    file.open(path.c_str());
    if(!file.is_open()) return false;
    std::string original_path = getAbsPath(path);

    while(!file.eof()) {
        getline(file, buffer);
        if (buffer.find("#include ") == 0 || buffer.find("#pragma include ") == 0){
            unsigned begin = buffer.find_first_of("\"");
            unsigned end = buffer.find_last_of("\"");
            if (begin != end) {
                std::string file_name = buffer.substr(begin+1,end-begin-1);
                file_name = urlResolve(file_name, original_path, include_folders);
                std::string newBuffer;
                if(loadFromPath(file_name, &newBuffer, include_folders)){
                    (*into) += "\n" + newBuffer + "\n";
                }
                else {
                    std::cout << file_name << " not found at " << original_path << std::endl;
                }
            }
        } else {
                (*into) += buffer + "\n";
        }
    }

    file.close();
    return true;
}

bool haveExt(const std::string& file, const std::string& ext){
    return file.find("."+ext) != std::string::npos;
}