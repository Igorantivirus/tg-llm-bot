#pragma once

#include <boost/asio/awaitable.hpp>
#include <openai/ChatsProcessor.hpp>
#include <tgbot/Types.h>
#include <tgbot/tgbot.h>

#include <app/Types.hpp>
#include <app/config/Locale.hpp>
#include <app/core/Operator.hpp>
#include <app/permissions/Editor.hpp>
#include <app/permissions/ReadWriter.hpp>
#include <app/transport/TgBotMessageSender.hpp>
#include <app/transport/presentation/Operation.hpp>
#include <utils/Format.hpp>

namespace handlers
{
class QueryProcessor
{
public:
    QueryProcessor(openai::ChatsProcessor &proc, transport::TgBotMessageSender &sender, config::Locale locale)
        : proc_(proc), sender_(sender), locale_(std::move(locale))
    {
    }

    asio::awaitable<void> onSetModelQuery(transport::Operation oper, TgBot::Message::Ptr msg, TgBot::CallbackQuery::Ptr query)
    {
        proc_.settings().repo().setModel(msg->chat->id, oper.data);
        co_await sender_.answerCallBackQuery(query->id, locale_.modelSetted);
        co_await sender_.editMessage(msg->chat->id, msg->messageId, utils::Format::format(locale_.currentModel, oper.data));
    }

private:
    openai::ChatsProcessor        &proc_;
    transport::TgBotMessageSender &sender_;

    config::Locale locale_;
};
} // namespace handlers