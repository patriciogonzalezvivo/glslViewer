#pragma once

#include <vector>
#include <string>
#include <functional>

struct Command {
    Command() {}

    Command(const std::string &_trigger, std::function<bool(const std::string&)> _do, const std::string &_formula, const std::string &_description, bool _mutex = true) {
        trigger = _trigger;
        exec = _do;
        formula = _formula;
        description = _description;
        mutex = _mutex;
    }

    std::string                             trigger;
    std::string                             formula;
    std::string                             description;
    std::function<bool(const std::string&)> exec;
    bool                                    mutex;
};

typedef std::vector<Command> CommandList;
