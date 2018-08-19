#pragma once

#include <map>
#include <string>
#include <functional>

#include "../gl/shader.h"

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
    bool                            present = true;
};

typedef std::map<std::string, UniformFunction> UniformFunctionsList;


