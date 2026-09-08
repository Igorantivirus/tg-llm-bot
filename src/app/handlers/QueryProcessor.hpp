#pragma once

#include "openai/dto/ChatCompletions/Request.hpp"
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
    QueryProcessor(transport::TgBotMessageSender &sender, core::Operator &oper, config::Locale locale)
        : sender_(sender), oper_(oper), locale_(std::move(locale))
    {
    }

    asio::awaitable<void> setModel(transport::Operation oper, TgBot::Message::Ptr msg, TgBot::CallbackQuery::Ptr query)
    {
        core::OperationInfo::Ptr info = std::make_shared<core::OperationInfo>(msg->chat->id);
        co_await oper_.setModel(std::move(info), oper.data);
        co_await sender_.answerCallBackQuery(query->id, locale_.modelSetted);
        co_await sender_.editMessage(msg->chat->id, msg->messageId, utils::Format::format(locale_.currentModel, oper.data));
    }
    asio::awaitable<void> setEffort(transport::Operation oper, TgBot::Message::Ptr msg, TgBot::CallbackQuery::Ptr query)
    {
        auto effortOpt = magic_enum::enum_cast<dto::ReasoningEffort>(oper.data);
        if (!effortOpt)
        {
            co_await sender_.answerCallBackQuery(query->id, "Ошибка");
            co_return;
        }
        dto::ReasoningEffort     effort = effortOpt.value();
        core::OperationInfo::Ptr info = std::make_shared<core::OperationInfo>(msg->chat->id);
        co_await oper_.setEffort(std::move(info), effort);
        co_await sender_.answerCallBackQuery(query->id, "Уровень установлен");
        co_await sender_.editMessage(msg->chat->id, msg->messageId, utils::Format::format("Установлен уровень размышления: {}", oper.data));
    }
    asio::awaitable<void> close(transport::Operation oper, TgBot::Message::Ptr msg, TgBot::CallbackQuery::Ptr query)
    {
        co_await sender_.deleteMessage(msg->chat->id, msg->messageId);
    }

private:
    transport::TgBotMessageSender &sender_;
    core::Operator                &oper_;
    config::Locale                 locale_;
};
} // namespace handlers