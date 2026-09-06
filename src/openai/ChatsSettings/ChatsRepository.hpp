#pragma once

#include "openai/dto/ChatCompletions/Request.hpp"
#include <unordered_map>
#include <unordered_set>

#include <openai/chatssettings/ChatHistory.hpp>

namespace openai
{
class ChatsRepository
{
public:
    ChatsRepository(std::string defaultModel, dto::ReasoningEffort defaultEffort)
        : defaultModel_(defaultModel), defaultEffort_(defaultEffort)
    {
    }

    void setModel(const ChatIdType id, std::string model)
    {
        histories_[id].model = std::move(model);
    }

    void setSystem(const ChatIdType id, std::string system)
    {
        histories_[id].system = std::move(system);
    }

    void setAlloTools(const ChatIdType id, std::unordered_set<std::string> allowTools)
    {
        histories_[id].allowTools = std::move(allowTools);
    }

    void clearHostory(const ChatIdType id)
    {
        histories_[id].history.clear();
    }

    void addDialogFragment(const ChatIdType id, DialogFragment fragment)
    {
        histories_[id].history.push_back(std::move(fragment));
    }

    void removeOldFragments(const ChatIdType id, const std::size_t count)
    {
        auto history = histories_[id];
        if (history.history.size() <= count)
            return history.history.clear();
        history.history.erase(history.history.begin(), history.history.begin() + count);
    }

    const ChatHistory &getHistoryById(const ChatIdType id)
    {
        auto [it, inserted] = histories_.insert({id, {}});
        if (inserted)
        {
            it->second.model = defaultModel_;
            it->second.effort = defaultEffort_;
        }
        return it->second;
    }

private:
    std::unordered_map<ChatIdType, ChatHistory> histories_;
    std::string                                 defaultModel_;
    dto::ReasoningEffort                        defaultEffort_;
};
} // namespace openai