#pragma once

#include <string>

#include "Types.hpp"

class MessageSender
{
public:
    virtual ~MessageSender() = default;

    virtual MessageID sendMessage(const ChatID chatId, std::string message) = 0;
    virtual void      addToMessage(const ChatID chatId, const MessageID msgId, std::string message) = 0;
    virtual void      replaceMessage(const ChatID chatId, const MessageID msgId, std::string message) = 0;
};