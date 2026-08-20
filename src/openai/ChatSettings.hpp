#pragma once

#include <string>
#include <vector>

#include "dto/Role.hpp"

struct ChatSettings
{
    std::string                                    model;
    std::string                                    systemPromt;
    std::vector<std::pair<dto::Role, std::string>> mesages;
};