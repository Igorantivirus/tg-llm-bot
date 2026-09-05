#pragma once

#include <ctime>
#include <string>
#include <vector>

namespace dto
{
struct Model
{
    std::string id;
    std::string object;
    std::time_t created;
    std::string owned_by;
};

struct ModelsResponse
{
    std::string        object;
    std::vector<Model> data;
};
} // namespace dto