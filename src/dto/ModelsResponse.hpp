#pragma once

#include <ctime>
#include <string>
#include <vector>

#include <utils/JsonSerialize.hpp>

namespace dto
{
struct Model
{
    std::string id;
    std::string object;
    std::time_t created;
    std::string owned_by;
};
JSONSER_SIMPLE_SERIALIZATION_OUTLINE_WITH_DEFAULTS(Model);

struct ModelsResponse
{
    std::string        object;
    std::vector<Model> data;
};
JSONSER_SIMPLE_SERIALIZATION_OUTLINE_WITH_DEFAULTS(ModelsResponse);
} // namespace dto