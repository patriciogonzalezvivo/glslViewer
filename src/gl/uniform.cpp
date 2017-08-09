#include "uniform.h"

#include <regex>
#include <sstream>

#include "../utils.h"

bool parseUniforms(const std::string &_line, UniformList *_uniforms) {
    bool rta = false;
    std::regex re("(\\w+)\\,");
    std::smatch match;
    if (std::regex_search(_line, match, re)) {
        // Extract uniform name
        std::string name = std::ssub_match(match[1]).str();

        // Extract values
        int index = 0;
        std::stringstream ss(_line);
        std::string item;
        while (getline(ss, item, ',')) {
            if (index != 0) {
                (*_uniforms)[name].value[index-1] = toFloat(item);
            }
            index++;
        }

        // Set total amount of values
        (*_uniforms)[name].size = index-1;
    }
    return rta;
}