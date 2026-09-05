#pragma once

#include <charconv>
#include <optional>

#include <tgbot/tgbot.h>

#include <app/Types.hpp>
#include <app/config/Locale.hpp>
#include <app/core/Operator.hpp>
#include <app/permissions/Editor.hpp>
#include <app/permissions/ReadWriter.hpp>
#include <app/transport/TgBotMessageSender.hpp>
#include <utils/Format.hpp>

namespace handlers
{
class CommandsProcessor
{
public:
    CommandsProcessor(core::Operator &op, permissions::Editor &editor, permissions::ReadWriter &readwriter, transport::TgBotMessageSender &sender, config::Locale locale)
        : operator_(op), editor_(editor), readwriter_(readwriter), sender_(sender), locale_(std::move(locale))
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
        if (args.size() == 1)
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
        co_await applyEdit(std::move(args), std::move(msg), &permissions::Editor::makeAdmin);
    }
    boost::asio::awaitable<void> removeAdmin(std::vector<std::string> args, TgBot::Message::Ptr msg)
    {
        co_await applyEdit(std::move(args), std::move(msg), &permissions::Editor::makeUnadmin);
    }
    boost::asio::awaitable<void> banUser(std::vector<std::string> args, TgBot::Message::Ptr msg)
    {
        co_await applyEdit(std::move(args), std::move(msg), &permissions::Editor::ban);
    }
    boost::asio::awaitable<void> unbanUser(std::vector<std::string> args, TgBot::Message::Ptr msg)
    {
        co_await applyEdit(std::move(args), std::move(msg), &permissions::Editor::unban);
    }
    boost::asio::awaitable<void> addGroup(std::vector<std::string> args, TgBot::Message::Ptr msg)
    {
        co_await applyEdit(std::move(args), std::move(msg), &permissions::Editor::addGroup, msg->chat->id, false);
    }
    boost::asio::awaitable<void> removeGroup(std::vector<std::string> args, TgBot::Message::Ptr msg)
    {
        co_await applyEdit(std::move(args), std::move(msg), &permissions::Editor::dellGroup, msg->chat->id, false);
    }
    boost::asio::awaitable<void> addChat(std::vector<std::string> args, TgBot::Message::Ptr msg)
    {
        co_await applyEdit(std::move(args), std::move(msg), &permissions::Editor::addPersonalChat, msg->chat->id);
    }
    boost::asio::awaitable<void> removeChat(std::vector<std::string> args, TgBot::Message::Ptr msg)
    {
        co_await applyEdit(std::move(args), std::move(msg), &permissions::Editor::dellPersonalChat, msg->chat->id);
    }

private:
    using EditMethod = bool (permissions::Editor::*)(const app::UserId);

    boost::asio::awaitable<void> applyEdit(std::vector<std::string> args, TgBot::Message::Ptr msg, EditMethod method, std::optional<app::UserId> defaultId = std::nullopt, const bool allowReply = true)
    {
        app::UserId id = {};
        if (!args.empty())
        {
            const std::string &str = args[0];
            auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), id);
            if (ec != std::errc())
            {
                std::ignore = (co_await sender_.sendMessage(msg->messageId, locale_.commandErrors.invalidUserId));
                co_return;
            }
        }
        else if (auto replyId = allowReply ? getReplyUserId(msg) : std::optional<app::UserId>{})
            id = *replyId;
        else if (defaultId)
            id = *defaultId;
        else
        {
            std::ignore = (co_await sender_.sendMessage(msg->messageId, locale_.commandErrors.argumentsError));
            co_return;
        }

        if (!(editor_.*method)(id))
        {
            std::ignore = (co_await sender_.sendMessage(msg->messageId, locale_.commandErrors.commandError));
            co_return;
        }
        readwriter_.save();
        std::ignore = (co_await sender_.sendMessage(msg->messageId, locale_.success));
        co_return;
    }

    static std::optional<app::UserId> getReplyUserId(const TgBot::Message::Ptr &msg)
    {
        if (!msg->replyToMessage || !msg->replyToMessage->from)
            return std::nullopt;
        return msg->replyToMessage->from->id;
    }

private:
    core::Operator                &operator_;
    permissions::Editor           &editor_;
    permissions::ReadWriter       &readwriter_;
    transport::TgBotMessageSender &sender_;

    config::Locale locale_;
};
} // namespace handlers