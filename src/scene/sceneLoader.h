#pragma once

#include <vector>

#include "model.h"
#include "../tools/uniform.h"

bool loadPLY(Uniforms& _uniforms, WatchFileList& _files, std::vector<Material>& _materials, std::vector<Model*>& _models, int _index, bool _verbose);
bool loadOBJ(Uniforms& _uniforms, WatchFileList& _files, std::vector<Material>& _materials, std::vector<Model*>& _models, int _index, bool _verbose);