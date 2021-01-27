
#pragma once

#include "texture.h"

class TextureStream : public Texture {
public:
    virtual bool    update() = 0;
};
