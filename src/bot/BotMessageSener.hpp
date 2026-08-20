#pragma once

#include <openai/MessageSender.hpp>
#include <tgbot/Api.h>

namespace bot
{

class BotMessageSener : public openai::MessageSender
{
public:
    BotMessageSener(const TgBot::Api &api)
        : api_(api)
    {
    }

    MessageID sendMessage(const ChatID chatId, std::string message) override
    {
        if(message.empty())
            return 0;
        auto msgPtr = api_.sendMessage(chatId, message);
        if (msgPtr)
            return msgPtr->messageId;
        return 0;
    }
    void replaceMessage(const ChatID chatId, const MessageID msgId, std::string message) override
    {
        api_.editMessageText(message, chatId, msgId);
    }
    void sendError(const ChatID chatId, utils::ErrorCode err) override
    {
        api_.sendMessage(chatId, err.message());
    }
    void replaceMessageWithError(const ChatID chatId, const MessageID msgId, std::string message, utils::ErrorCode err) override
    {
        message += "\nError: " + err.message();
        replaceMessage(chatId, msgId, std::move(message));
    }

private:
    const TgBot::Api &api_;
};

} // namespace bot