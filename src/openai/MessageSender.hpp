#pragma once

#include <string>
#include <variant>

#include <utils/Types.hpp>

#include "Types.hpp"

namespace openai
{

class MessageSender
{
public:
    using Error = std::variant<utils::ErrorCode, std::string>;

public:
    virtual ~MessageSender() = default;

    virtual MessageID sendMessage(const ChatID chatId, std::string message) = 0;
    virtual void      addToMessage(const ChatID chatId, const MessageID msgId, std::string message) = 0;
    virtual void      replaceMessage(const ChatID chatId, const MessageID msgId, std::string message) = 0;

    virtual MessageID sendError(const ChatID chatId, Error ec) = 0;
    virtual void      addToError(const ChatID chatId, const MessageID msgId, Error ec) = 0;
    virtual void      replaceError(const ChatID chatId, const MessageID msgId, Error ec) = 0;

protected:
    static std::string toString(const Error &er)
    {
        if (er.index() == 0)
            return std::get<0>(er).message();
        else
            return std::get<1>(er);
    }
};

} // namespace openai