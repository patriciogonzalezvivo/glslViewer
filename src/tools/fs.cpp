#include <iostream>     // cout
#include <fstream>      // File
#include <iterator>     // std::back_inserter
#include <algorithm>    // std::unique

#include <sys/stat.h>

#include "fs.h"


bool haveExt(const std::string& _file, const std::string& _ext){
    return _file.find( "." + _ext) != std::string::npos;
}

bool urlExists(const std::string& name) {
    struct stat buffer;
    return (stat (name.c_str(), &buffer) == 0);
}

std::string getBaseDir(const std::string& filepath) {
    std::string base_dir = "";

    if (filepath.find_last_of("/\\") != std::string::npos)
        base_dir =  filepath.substr(0, filepath.find_last_of("/\\"));
    else 
        base_dir = ".";
    
#ifdef PLATFORM_WIN32
    base_dir += "\\";
#else
    base_dir += "/";
#endif

    return base_dir;
}

std::string getAbsPath(const std::string& _path) {
    std::string abs_path = realpath(_path.c_str(), NULL);
    std::size_t found = abs_path.find_last_of("\\/");
    if (found) return abs_path.substr(0, found);
    else return "";
}

std::string urlResolve(const std::string& _path, const std::string& _pwd, const List &_include_folders) {
    std::string url = _pwd +'/'+ _path;

    // If the path is not in the same directory
    if (urlExists(url)) 
        return realpath(url.c_str(), NULL);

    // .. search on the include path
    else {
        for (uint i = 0; i < _include_folders.size(); i++) {
            std::string new_path = _include_folders[i] + "/" + _path;
            if (urlExists(new_path)) 
                return realpath(new_path.c_str(), NULL);
        }
        return _path;
    }
}

bool extractDependency(const std::string &_line, std::string *_dependency) {
    // Search for ocurences of #include or #pargma include (OF)
    if (_line.find("#include ") == 0 || _line.find("#pragma include ") == 0) {
        unsigned begin = _line.find_first_of("\"");
        unsigned end = _line.find_last_of("\"");
        if (begin != end) {
            (*_dependency) = _line.substr(begin+1,end-begin-1);
            return true;
        }
    }
    return false;
}

bool alreadyInclude(const std::string &_path, List *_dependencies) {
    for (unsigned int i = 0; i < _dependencies->size(); i++) {
        if ( _path == (*_dependencies)[i]) {
            return true;
        }
    }
    return false;
}

bool loadFromPath(const std::string &_path, std::string *_into, const std::vector<std::string> &_include_folders, List *_dependencies) {
    std::ifstream file;
    file.open(_path.c_str());

    // Skip if it's already open
    if (!file.is_open()) 
        return false;

    // Get absolute home folder
    std::string original_path = getAbsPath(_path);

    std::string line;
    std::string dependency;
    std::string newBuffer;
    while (!file.eof()) {
        dependency = "";
        getline(file, line);

        if (extractDependency(line, &dependency)) {
            dependency = urlResolve(dependency, original_path, _include_folders);
            newBuffer = "";
            if (loadFromPath(dependency, &newBuffer, _include_folders, _dependencies)) {
                if (!alreadyInclude(dependency, _dependencies)) {
                    // Insert the content of the dependency
                    (*_into) += "\n" + newBuffer + "\n";

                    // Add dependency to dependency list
                    _dependencies->push_back(dependency);
                }
            }
            else {
                std::cerr << "Error: " << dependency << " not found at " << original_path << std::endl;
            }
        }
        else {
            (*_into) += line + "\n";
        }
    }

    file.close();
    return true;
}

std::string toString(FileType _type) {
    if (_type == FRAG_SHADER) {
        return "FRAG_SHADER";
    }
    else if (_type == VERT_SHADER) {
        return "VERT_SHADER";
    }
    else if (_type == IMAGE) {
        return "IMAGE";
    }
    else if (_type == GEOMETRY) {
        return "GEOMETRY";
    }
    else if (_type == CUBEMAP) {
        return "CUBEMAP";
    }
    else if (_type == GLSL_DEPENDENCY) {
        return "GLSL_DEPEND";
    }
    else {
        return "-undefined-";
    }
}