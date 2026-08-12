#pragma once

#include <unordered_map>

#include "AuthorType.hpp"
#include "MessageHistory.hpp"
#include "Types.hpp"

class HistoryRepository
{
public:
    void clearHistory(const ChatID chatId)
    {
        auto found = history_.find(chatId);
        if (found == history_.end())
            return;
        found->second.mesages.clear();
    }
    void setSystemPromt(const ChatID chatId, std::string system)
    {
        MessageHistory &msgs = history_[chatId];
        msgs.systemPromt = std::move(system);
    }
    void addMessage(const ChatID chatId, std::string msg, AuthorType type)
    {
        MessageHistory &msgs = history_[chatId];
        msgs.mesages.push_back(std::make_pair(type, std::move(msg)));
    }
    const MessageHistory &getHistory(const ChatID chatId)
    {
        return history_[chatId];
    }

private:
    std::unordered_map<ChatID, MessageHistory> history_;
};