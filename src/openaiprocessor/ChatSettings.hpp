#pragma once

#include <string>
#include <vector>

#include "AuthorType.hpp"

struct ChatSettings
{
    std::string systemPromt;
    std::vector<std::pair<AuthorType, std::string>> mesages;

    std::string model;
};