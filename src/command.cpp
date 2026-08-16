#include "command.hpp"
#include <sstream>

Command parse_command(const std::string& line) {
    Command cmd;
    std::istringstream iss(line);
    std::string token;

    iss >> cmd.verb;
    while (iss >> token) {
        cmd.args.push_back(token);
    }
    return cmd;
}