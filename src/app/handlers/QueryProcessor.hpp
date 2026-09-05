#pragma once

#include <boost/asio/awaitable.hpp>
#include <openai/ChatsProcessor.hpp>
#include <tgbot/Types.h>
#include <tgbot/tgbot.h>

#include <app/Types.hpp>
#include <app/core/Operator.hpp>
#include <app/permissions/Editor.hpp>
#include <app/permissions/ReadWriter.hpp>
#include <app/transport/TgBotMessageSender.hpp>
#include <app/transport/presentation/Operation.hpp>

namespace handlers
{
class QueryProcessor
{
public:
    QueryProcessor(openai::ChatsProcessor &proc, transport::TgBotMessageSender &sender)
        : proc_(proc), sender_(sender)
    {
    }

    asio::awaitable<void> onQuery(transport::Operation oper, TgBot::Message::Ptr msg, TgBot::CallbackQuery::Ptr query)
    {
        if (oper.type == transport::OperationType::SetMdl)
        {
            proc_.settings().repo().setModel(msg->chat->id, oper.data);
            co_await sender_.answerCallBackQuery(query->id, "Модель установлена.");
            co_await sender_.editMessage(msg->chat->id, msg->messageId, "Текущая модель: " + oper.data);
        }
        co_return;
    }

private:
    openai::ChatsProcessor        &proc_;
    transport::TgBotMessageSender &sender_;
};
} // namespace handlers