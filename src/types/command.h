#pragma once

#include <vector>
#include <string>
#include <functional>

struct Command {
    Command() {}

    Command(const std::string &_begins_width, std::function<bool(const std::string&)> _do, const std::string &_description, bool _mutex = true) {
        begins_with = _begins_width;
        exec = _do;
        description = _description;
        mutex = _mutex;
    }

    std::string                             begins_with;
    std::string                             description;
    std::function<bool(const std::string&)> exec;
    bool                                    mutex;
};

typedef std::vector<Command> CommandList;
