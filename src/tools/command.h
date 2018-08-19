#pragma once

#include <vector>
#include <string>
#include <functional>

struct Command {
    Command() {}

    Command(const std::string &_begins_width, std::function<bool(const std::string&)> _do) {
        begins_with = _begins_width;
        exec = _do;
    }

    Command(const std::string &_begins_width, std::function<bool(const std::string&)> _do, const std::string &_description ) {
        begins_with = _begins_width;
        exec = _do;
        description = _description;
    }


    std::string                             begins_with;
    std::function<bool(const std::string&)> exec;
    std::string                             description;
};

typedef std::vector<Command> CommandList;
