#pragma once

#include "openai/dto/ChatCompletions/Request.hpp"
#include <unordered_set>

#include <openai/chatssettings/Types.hpp>

namespace openai
{
struct ChatHistory
{
    std::string                 model;
    std::string                 system;
    dto::ReasoningEffort        effort;
    std::vector<DialogFragment> history;

    std::unordered_set<std::string> allowTools;
};

} // namespace openai