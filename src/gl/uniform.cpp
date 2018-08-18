#include "uniform.h"

#include <regex>
#include <sstream>

#include "tools/text.h"

std::string UniformData::getType() {
    if (size == 1 && bInt) {
        return "int";
    }
    else if (size == 1 && !bInt) {
        return "float";
    }
    else {
        return (bInt ? "ivec" : "vec") + toString(size); 
    }
}

UniformFunction::UniformFunction() {

}

UniformFunction::UniformFunction(const std::string &_type, const std::string &_description) {
    type = _type;
    description = _description;
}

bool parseUniformData(const std::string &_line, UniformDataList *_uniforms) {
    bool rta = false;
    std::regex re("^(\\w+)\\,");
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
                (*_uniforms)[name].bInt = !isFloat(item);
                (*_uniforms)[name].value[index-1] = toFloat(item);
            }
            index++;
        }

        // Set total amount of values
        (*_uniforms)[name].size = index-1;
    }
    return rta;
}
