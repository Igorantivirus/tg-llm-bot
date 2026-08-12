#pragma once

#include <string_view>

#include "MessageSender.hpp"
#include "Types.hpp"

class MessageProcessor
{
public:
    MessageProcessor(MessageSender &sender)
        : sender_(sender)
    {
    }

    void processMessage(const ChatID chatId, const std::string_view msg)
    {
        sender_.sendMessage(chatId, "Вот мой ответ!\nЛютая месть!");
    }

private:
    MessageSender &sender_;
};