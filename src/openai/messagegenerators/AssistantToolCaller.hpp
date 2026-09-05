#pragma once

#include <openai/Tools/Tool.hpp>
#include <openai/dto/ChatCompletions/Message.hpp>

namespace openai
{
class AssistantToolCaller
{
public:
    AssistantToolCaller(const std::unordered_map<std::string, Tool::Ptr> &tools)
        : tools_(tools)
    {
    }

    // Вызывающий гарантирует, что все msg.tool_calls[i] - валидные инструменты, которые содержат все поля (non std::nullopt)
    asio::awaitable<std::vector<dto::Message>> callsTools(const dto::Message &msg)
    {
        std::vector<dto::Message> result;
        if (!msg.tool_calls)
            co_return result;
        for (const dto::ToolCall &tcl : msg.tool_calls.value())
        {
            auto found = tools_.find(tcl.function->name.value());
            if (found == tools_.end())
                continue;
            result.push_back(co_await callToolToMessage(found->second, tcl.function->arguments.value(), tcl.id.value()));
        }
        co_return result;
    }

private:
    const std::unordered_map<std::string, Tool::Ptr> &tools_;

private:
    static asio::awaitable<dto::Message> callToolToMessage(const Tool::Ptr &tool, const std::string &args, std::string toolCallId)
    {
        auto         callResult = co_await tool->run(args);
        dto::Message msg;
        msg.role = dto::Role::tool;
        msg.tool_call_id = toolCallId;
        msg.content = callResult ? callResult.value() : callResult.error().message();
        co_return msg;
    }
};
} // namespace openai