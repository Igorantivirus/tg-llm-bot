#pragma once

#include "TgBotApiRedirector.hpp"
#include "core/Operator.hpp"
#include <boost/asio/awaitable.hpp>
#include <memory>
#include <tgbot/Types.h>

namespace middleware
{
class MessagesProcessor
{
public:
    MessagesProcessor(core::Operator &op, TgBotApiRedirector &redirector)
        : operator_(op), redirector_(redirector)
    {
    }

    asio::awaitable<void> addMessage(TgBot::Message::Ptr msg)
    {
        if (!msg->text)
            co_return;
        core::OperationInfo::Ptr info = std::make_shared<core::OperationInfo>(msg->chat->id);
        co_await operator_.addMessage(info, msg->text.value());
    }

    asio::awaitable<void> processMessage(TgBot::Message::Ptr msg)
    {
        if (!msg->text)
            co_return;
        // TODO: downlaod files and photos
        core::OperationInfo::Ptr info = std::make_shared<core::OperationInfo>(msg->chat->id);
        co_await operator_.processMessage(info, msg->text.value());
    }

private:
    core::Operator     &operator_;
    TgBotApiRedirector &redirector_;
};
} // namespace middleware