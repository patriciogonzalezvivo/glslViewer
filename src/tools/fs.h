#pragma once

#include <vector>
#include <string>

enum FileType {
    FRAG_SHADER     = 0,
    VERT_SHADER     = 1,
    IMAGE           = 2,
    GEOMETRY        = 3,
    CUBEMAP         = 4,
    GLSL_DEPENDENCY = 5
};

struct WatchFile {
    std::string path;
    FileType    type;
    int         lastChange;
    bool        vFlip;      // Use for textures to know if they should be flipped or not
};

typedef std::vector<std::string> FileList;
typedef std::vector<WatchFile> WatchFileList;

bool urlExists(const std::string &_filename);
bool haveExt(const std::string &_filename, const std::string &_ext);

bool loadFromPath(const std::string &_filename, std::string *_into, const FileList &_include_folders, FileList *_dependencies);

std::string toString(FileType _type);
std::string getBaseDir (const std::string& filepath);
std::string getAbsPath (const std::string &_filename);
std::string urlResolve(const std::string &_filename, const std::string &_pwd, const FileList &_include_folders);

FileList mergeList(const FileList &_A, const FileList &_B);


