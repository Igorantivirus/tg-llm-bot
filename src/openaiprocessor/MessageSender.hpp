#pragma once

#include <string>

#include "Types.hpp"

class MessageSender
{
public:
    virtual ~MessageSender() = default;

    virtual void startSendMessages(const ChatID chatId) = 0;
    virtual void stopSendMessages(const ChatID chatId) = 0;
    virtual void sendNextMessage(const ChatID chatId, const std::string& msg) = 0;
    
    virtual void sendMessage(const ChatID chatId, const std::string& msg) = 0;
};