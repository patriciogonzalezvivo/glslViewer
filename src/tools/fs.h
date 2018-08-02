#pragma once

#include <vector>
#include <string>

struct WatchFile {
    std::string type;
    std::string path;
    bool vFlip;
    int lastChange;
};

typedef std::vector<WatchFile> WatchFileList;

std::string getAbsPath (const std::string& str);
bool urlExists(const std::string& name);
std::string urlResolve(const std::string& path, const std::string& pwd, const std::vector<std::string> include_folders);
bool loadFromPath(const std::string& path, std::string* into, const std::vector<std::string> include_folders);
bool haveExt(const std::string& file, const std::string& ext);