#include "gltf.h"

#include <iostream>
#include <fstream>
#include <string>

#include "../tools/geom.h"
#include "../tools/text.h"

#define TINYGLTF_IMPLEMENTATION
// #define STB_IMAGE_IMPLEMENTATION
// #define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tinygltf/tiny_gltf.h"


bool loadGLTF(Uniforms& _uniforms, WatchFileList& _files, std::map<std::string,Material>& _materials, std::vector<Model*>& _models, int _index, bool _verbose) {
    return true;
}
