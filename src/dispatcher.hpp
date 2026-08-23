#pragma once
#include "command.hpp"
#include "store.hpp"

std::string dispatch(const Command& cmd, Store& store);