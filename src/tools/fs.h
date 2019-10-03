#pragma once

#include "list.h"

enum FileType {
    FRAG_SHADER     = 0,
    VERT_SHADER     = 1,
    IMAGE           = 2,
    GEOMETRY        = 3,
    CUBEMAP         = 4,
    GLSL_DEPENDENCY = 5,
    IMAGE_BUMPMAP   = 6
};

struct WatchFile {
    std::string path;
    FileType    type;
    int         lastChange;
    bool        vFlip;      // Use for textures to know if they should be flipped or not
};

typedef std::vector<WatchFile> WatchFileList;

bool urlExists(const std::string& _filename);
bool haveExt(const std::string& _filename, const std::string& _ext);
std::string getExt(const std::string& _filename);

bool loadFromPath(const std::string& _filename, std::string *_into, const List& _include_folders, List *_dependencies);

std::string toString(FileType _type);
std::string getBaseDir (const std::string& filepath);
std::string getAbsPath (const std::string& _filename);
std::string urlResolve(const std::string& _filename, const std::string& _pwd, const List& _include_folders);


