#pragma once

#include <vector>

#include "model.h"
#include "../uniform.h"

bool loadPLY(Uniforms& _uniforms, WatchFileList& _files, std::map<std::string,Material>& _materials, std::vector<Model*>& _models, int _index, bool _verbose);
bool loadOBJ(Uniforms& _uniforms, WatchFileList& _files, std::map<std::string,Material>& _materials, std::vector<Model*>& _models, int _index, bool _verbose);