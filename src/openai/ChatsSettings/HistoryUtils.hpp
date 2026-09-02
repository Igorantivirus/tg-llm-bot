#pragma once

#include <dto/ChatCompletions/Request.hpp>
#include <ranges>

#include "ChatHistory.hpp"
#include "ChatsSettings.hpp"
#include "Types.hpp"
#include "dto/ChatCompletions/Message.hpp"

namespace openai
{
class HistoryUtils
{
public:
    static void appendFragmentToMessages(std::vector<dto::Message> &messages, const DialogFragment &fragment)
    {
        for (const auto &msg : fragment)
            messages.push_back(msg);
    }

    static void historyToMessages(std::vector<dto::Message> &messages, const ChatHistory &history)
    {
        messages.reserve(getSize(history));

        dto::Message system;
        system.content = history.system;
        system.role = dto::Role::system;
        messages.push_back(std::move(system));

        for (const auto &fragment : history.history)
            appendFragmentToMessages(messages, fragment);
    }

    static dto::ChatCompletionsRequest makeRequest(const ChatsSettings &setts, const ChatHistory &history, const DialogFragment &processFragment)
    {
        dto::ChatCompletionsRequest req; // Ставим основное
        req.stream = setts.stream();
        req.model = history.model;

        req.messages.reserve(getSize(history) + processFragment.size());
        historyToMessages(req.messages, history);
        appendFragmentToMessages(req.messages, processFragment);

        if (!setts.tools().empty())
            req.tools = setts.tools() | std::views::transform([](const auto &pair) -> dto::Tool
            {
                return pair.second->dto();
            }) | std::views::filter([&history](const dto::Tool &tool) -> bool
            {
                return history.allowTools.count(tool.function.name) > 0;
            }) | std::ranges::to<std::vector<dto::Tool>>();
        if (!setts.stops().empty())
            req.stop = setts.stops() | std::ranges::to<std::vector<std::string>>();
        return req;
    }

private:
    static std::size_t getSize(const ChatHistory &history)
    {
        std::size_t size = 0;
        for (const auto &frag : history.history)
            size += frag.size();
        return size + !history.system.empty();
    }
};
} // namespace openai