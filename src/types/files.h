#pragma once

#include <string>
#include <vector>

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