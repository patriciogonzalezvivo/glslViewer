#pragma once

#include <vector>

#include "../uniform.h"
#include "../scene/model.h"

bool loadOBJ(Uniforms& _uniforms, WatchFileList& _files, std::map<std::string,Material>& _materials, std::vector<Model*>& _models, int _index, bool _verbose);