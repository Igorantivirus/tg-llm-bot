#pragma once

#include <tgbot/tgbot.h>

#include <app/Types.hpp>
#include <app/core/Operator.hpp>
#include <app/permissions/Editor.hpp>
#include <app/permissions/ReadWriter.hpp>
#include <app/transport/TgBotMessageSender.hpp>

namespace handlers
{
class CommandsProcessor
{
public:
    CommandsProcessor(core::Operator &op, permissions::Editor &editor, permissions::ReadWriter &readwriter, transport::TgBotMessageSender &sender)
        : operator_(op), editor_(editor), readwriter_(readwriter), sender_(sender)
    {
    }

    asio::awaitable<void> clearCommand(std::vector<std::string>, TgBot::Message::Ptr msg)
    {
        core::OperationInfo::Ptr info = std::make_shared<core::OperationInfo>(msg->chat->id);
        co_await operator_.clear(info);
    }

    boost::asio::awaitable<void> stopCommand(std::vector<std::string>, TgBot::Message::Ptr msg)
    {
        core::OperationInfo::Ptr info = std::make_shared<core::OperationInfo>(msg->chat->id);
        co_await operator_.stop(info);
    }

    boost::asio::awaitable<void> stopAllCommand(std::vector<std::string>, TgBot::Message::Ptr msg)
    {
        core::OperationInfo::Ptr info = std::make_shared<core::OperationInfo>(msg->chat->id);
        co_await operator_.stopAll(info);
    }

    boost::asio::awaitable<void> modelsCommand(std::vector<std::string>, TgBot::Message::Ptr msg)
    {
        core::OperationInfo::Ptr info = std::make_shared<core::OperationInfo>(msg->chat->id);
        co_await operator_.models(info);
    }

    boost::asio::awaitable<void> modelCommand(std::vector<std::string> args, TgBot::Message::Ptr msg)
    {
        core::OperationInfo::Ptr info = std::make_shared<core::OperationInfo>(msg->chat->id);
        if (!args.empty())
            co_await operator_.setModel(info, std::move(args[0]));
        co_await operator_.model(info);
    }
    boost::asio::awaitable<void> systemCommand(std::vector<std::string> args, TgBot::Message::Ptr msg)
    {
        core::OperationInfo::Ptr info = std::make_shared<core::OperationInfo>(msg->chat->id);
        if (args.size() == 1)
            co_await operator_.setSystem(info, std::move(args[0]));
        else
            co_await operator_.getSystem(info);
    }

    boost::asio::awaitable<void> makeAdmin(std::vector<std::string> args, TgBot::Message::Ptr msg)
    {
        app::UserId id = {};
        if (!args.empty())
        {
            const std::string &str = args[0];
            auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), id);
            if (ec != std::errc())
                co_return (co_await sender_.sendMessage(msg->messageId, "Неверный user id."));
        }
        else
            co_return (co_await sender_.sendMessage(msg->messageId, "Ошибка."));

        if (!editor_.makeAdmin(id))
            co_return (co_await sender_.sendMessage(msg->messageId, "Ошибка"));
        readwriter_.save();
        co_return (co_await sender_.sendMessage(msg->messageId, "Успех."));
    }

private:
    core::Operator                &operator_;
    permissions::Editor           &editor_;
    permissions::ReadWriter       &readwriter_;
    transport::TgBotMessageSender &sender_;
};
} // namespace handlers