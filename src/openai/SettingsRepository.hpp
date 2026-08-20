#pragma once

#include <unordered_map>
#include <unordered_set>

#include "ChatSettings.hpp"
#include "Types.hpp"

class SettingsRepository
{
public:
    void setModels(std::unordered_set<std::string> models)
    {
        models_ = std::move(models);
        for (auto &[id, setts] : history_)
            if (models_.count(setts.model) == 0)
                setts.model.clear();
    }
    bool setModelToChat(const ChatID chatId, std::string model)
    {
        if (models_.count(model) == 0)
            return false;
        ChatSettings &setts = history_[chatId];
        setts.model = std::move(model);
        return true;
    }
    void setSystemPromt(const ChatID chatId, std::string system)
    {
        ChatSettings &setts = history_[chatId];
        setts.systemPromt = std::move(system);
    }
    
    void addMessage(const ChatID chatId, std::string msg, dto::Role type)
    {
        ChatSettings &msgs = history_[chatId];
        msgs.mesages.push_back(std::make_pair(type, std::move(msg)));
    }

    void clearHistory(const ChatID chatId)
    {
        if (auto found = history_.find(chatId); found != history_.end())
            found->second.mesages.clear();
    }

    const ChatSettings &getSettings(const ChatID chatId)
    {
        return history_[chatId];
    }
    const std::unordered_set<std::string> &getModels() const
    {
        return models_;
    }

private:
    std::unordered_map<ChatID, ChatSettings> history_;
    std::unordered_set<std::string>          models_;
};