#include <string>
#include <map>

#include "shader.h"

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
    UniformFunction(const std::string &_type, const std::string &_description);

    std::string type;
    std::string description;
    bool        present    = true;
};

typedef std::map<std::string, UniformFunction> UniformFunctionsList;


