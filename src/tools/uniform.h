#pragma once

#include <map>
#include <vector>
#include <string>
#include <functional>

#include "../gl/fbo.h"
#include "../gl/shader.h"
#include "../gl/texture.h"

struct UniformData {
    std::string getType();

    float   value[4];
    int     size;
    bool    bInt = false;
    bool    change = false;
};
typedef std::map<std::string, UniformData> UniformDataList;
bool parseUniformData(const std::string &_line, UniformDataList *_uniforms);

struct UniformFunction {
    UniformFunction();
    UniformFunction(const std::string &_type);
    UniformFunction(const std::string &_type, std::function<void(Shader&)> _assign);
    UniformFunction(const std::string &_type, std::function<void(Shader&)> _assign, std::function<std::string()> _print);

    std::function<void(Shader&)>    assign;
    std::function<std::string()>    print;
    std::string                     type;
    bool                            present = false;
};

typedef std::map<std::string, UniformFunction> UniformFunctionsList;

typedef std::map<std::string, Texture*> TextureList;

class Uniforms {
public:
    Uniforms();
    virtual ~Uniforms();

    bool                    parseLine( const std::string &_line );
    void                    checkPresenceIn( const std::string &_vert_src, const std::string &_frag_src );
    bool                    feedTo( Shader &_shader );
    void                    print(bool _all);
    void                    printBuffers();
    void                    printTextures();

    void                    flagChange();
    void                    unflagChange();
    bool                    haveChange();
    void                    clear();

    UniformDataList         data;
    TextureList             textures;
    UniformFunctionsList    functions;
    std::vector<Fbo>        buffers;

protected:
    bool                    m_change;
};


