#pragma once

#include <string>
#include <vector>

#include "AuthorType.hpp"

struct MessageHistory
{
    std::string systemPromt;
    std::vector<std::pair<AuthorType, std::string>> mesages;
};