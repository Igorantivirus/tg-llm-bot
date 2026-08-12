#pragma once

#include "MessageSender.hpp"
#include "SettingsRepository.hpp"
#include "Types.hpp"

class MessageProcessor
{
public:
    MessageProcessor(MessageSender &sender)
        : sender_(sender)
    {
    }

    void processMessage(const ChatID chatId, std::string msg)
    {
        settings_.addMessage(chatId, std::move(msg), AuthorType::User);
        const ChatSettings &setts = settings_.getHistory(chatId);
        // TODO
    }

private:
    SettingsRepository settings_;
    MessageSender &sender_;
};