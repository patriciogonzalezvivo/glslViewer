#pragma once

#include <vector>

#include "model.h"
#include "../tools/fs.h"

bool loadPLY(std::vector<Model*>& _models, WatchFileList& _files, int _index, bool _verbose);
bool loadOBJ(std::vector<Model*>& _models, WatchFileList& _files, int _index, bool _verbose);