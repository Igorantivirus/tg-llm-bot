#pragma once

#include <unordered_set>

#include "Types.hpp"

namespace openai
{
struct ChatHistory
{
    std::string                 model;
    std::string                 system;
    std::vector<DialogFragment> history;

    std::unordered_set<std::string> allowTools;
};

} // namespace openai