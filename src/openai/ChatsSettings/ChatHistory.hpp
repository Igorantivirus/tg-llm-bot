#pragma once

#include <unordered_set>

#include <openai/chatssettings/Types.hpp>

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