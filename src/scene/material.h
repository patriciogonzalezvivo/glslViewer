#pragma once

#include <map>
#include <string>
#include "glm/glm.hpp"

#include "../uniform.h"
#include "../defines.h"
#include "../tools/fs.h"
#include "../gl/shader.h"

class Material : public HaveDefines {
public:
    Material();
    virtual ~Material();

    void feedProperties(Shader& _shader) const;
    
    std::string name;

};

typedef std::map<std::string,Material> Materials;
