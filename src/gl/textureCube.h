
#pragma once

#include "texture.h"
#include "glm/glm.hpp"
#include "types/skybox.h"

class TextureCube : public Texture {
public:
    TextureCube();
    virtual ~TextureCube();

    virtual bool    load(const std::string &_fileName, bool _vFlip = true, TextureFilter _filter = LINEAR, TextureWrap _wrap = REPEAT);
    virtual bool    generate(SkyBox* _skybox, int _width = 512 );

    virtual void    bind();

    glm::vec3       SH[9];
};

