#pragma once

#include <openaiprocessor/MessageSender.hpp>
#include <tgbot/Api.h>

class BotMessageSener : public MessageSender
{
public:
    BotMessageSener(const TgBot::Api &api)
        : api_(api)
    {
    }

    void startSendMessages(const ChatID chatId) override
    {
    }
    void stopSendMessages(const ChatID chatId) override
    {
    }
    void sendNextMessage(const ChatID chatId, const std::string &msg) override
    {
    }
    void sendMessage(const ChatID chatId, const std::string &msg) override
    {
        api_.sendMessage(chatId, msg);
    }

private:
    const TgBot::Api &api_;
};