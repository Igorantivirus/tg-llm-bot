#pragma once

#include <string>

#include <utils/Types.hpp>

#include "Types.hpp"

namespace openai
{

class MessageSender
{
public:
    virtual ~MessageSender() = default;

    virtual MessageID sendMessage(const ChatID chatId, std::string message) = 0;
    virtual void      replaceMessage(const ChatID chatId, const MessageID msgId, std::string message) = 0;
    virtual void      sendError(const ChatID chatId, utils::ErrorCode err) = 0;
    virtual void      replaceMessageWithError(const ChatID chatId, const MessageID msgId, std::string message, utils::ErrorCode err) = 0;
};

} // namespace openai