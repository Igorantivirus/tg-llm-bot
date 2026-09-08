#pragma once

#include "openai/Tools/ToolResult.hpp"
#include "openai/chatssettings/Types.hpp"
#include <openai/Tools/Tool.hpp>
#include <openai/dto/ChatCompletions/Message.hpp>
#include <utility>

namespace openai
{
class AssistantToolCaller
{
public:
    struct CallsResult
    {
        DialogFragment               fragment;
        std::vector<ToolResult::Ptr> results;
    };

public:
    AssistantToolCaller(const std::unordered_map<std::string, Tool::Ptr> &tools)
        : tools_(tools)
    {
    }

    // Вызывающий гарантирует, что все msg.tool_calls[i] - валидные инструменты, которые содержат все поля (non std::nullopt)
    asio::awaitable<CallsResult> callsTools(const dto::Message &msg)
    {
        CallsResult result;
        if (!msg.tool_calls)
            co_return result;
        for (const dto::ToolCall &tcl : msg.tool_calls.value())
        {
            auto found = tools_.find(tcl.function->name.value());
            if (found == tools_.end())
                continue;
            auto [dto, ptr] = co_await callToolToMessage(found->second, tcl.function->arguments.value(), tcl.id.value());
            result.fragment.push_back(std::move(dto));
            result.results.push_back(std::move(ptr));
        }
        co_return result;
    }

private:
    const std::unordered_map<std::string, Tool::Ptr> &tools_;

private:
    static asio::awaitable<std::pair<dto::Message, ToolResult::Ptr>> callToolToMessage(const Tool::Ptr &tool, const std::string &args, std::string toolCallId)
    {
        auto         callResult = co_await tool->run(args);
        dto::Message msg;
        msg.role = dto::Role::tool;
        msg.tool_call_id = toolCallId;
        msg.content = callResult ? callResult.value()->toString() : callResult.error().message();
        co_return std::make_pair(msg, callResult ? callResult.value() : nullptr);
    }
};
} // namespace openai