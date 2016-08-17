#include <string>
#include <map>

struct Uniform {
    float value[4];
    int size;
};
typedef std::map<std::string, Uniform> UniformList;

bool parseUniforms(const std::string &_line, UniformList *_uniforms);
