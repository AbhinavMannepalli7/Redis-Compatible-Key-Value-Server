#pragma once
#include <string>
#include <vector>

struct Command {
    std::string verb;
    std::vector<std::string> args;
};

Command parse_command(const std::string& line);