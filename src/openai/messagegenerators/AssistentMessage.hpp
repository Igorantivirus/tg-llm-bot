#pragma once

#include <string>
#include <vector>

#include <openai/Tools/ToolResult.hpp>

namespace openai
{
struct AssistentMessage
{
    std::string                  content;
    std::string                  reasoning;
    std::vector<ToolResult::Ptr> toolCallResult;
    bool                         toolCalling = false;

    bool empty() const
    {
        return content.empty() && reasoning.empty() && toolCallResult.empty();
    }
};
} // namespace openai