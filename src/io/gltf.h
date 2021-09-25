#pragma once

#include "../uniforms.h"
#include "ada/scene/model.h"

bool loadGLTF(Uniforms& _uniforms, WatchFileList& _files, ada::Materials& _materials, ada::Models& _models, int _index, bool _verbose);