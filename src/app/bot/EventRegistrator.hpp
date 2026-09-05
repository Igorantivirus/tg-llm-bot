#pragma once

#include <cstdint>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/awaitable.hpp>

#include "PermissionChecker.hpp"
#include <app/middleware/CommandsProcessor.hpp>
#include <app/middleware/MessagesProcessor.hpp>
#include <tgbot/Types.h>

namespace bot
{
class EventRegistrator
{
public:
    using CommandHandler = boost::asio::awaitable<void> (middleware::CommandsProcessor::*)(std::vector<std::string>, TgBot::Message::Ptr);
    using MessageHandler = boost::asio::awaitable<void> (middleware::MessagesProcessor::*)(TgBot::Message::Ptr);
    using Permission = bool                             (PermissionChecker::*)(const TgBot::Chat::Type, const middleware::ChatId, const middleware::UserId);

    struct CommandSettings
    {
        std::uint8_t minArgsCount = 0;
        std::uint8_t maxArgsCount = 255;
        bool         glueArgs = false;
    };

public:
    EventRegistrator(boost::asio::any_io_executor ex, TgBot::Bot &bot, PermissionChecker checker, middleware::CommandsProcessor &cmdProc, middleware::MessagesProcessor &msgProc)
        : ex_(ex), bot_(bot), checker_(checker), cmdProc_(cmdProc), msgProc_(msgProc)
    {
        me_ = bot_.getApi().getMe()->username.value_or("");
    }

    void registrateCommand(const std::string &command, CommandHandler handler, Permission permission, CommandSettings setts)
    {
        auto lambda = [this, handler = std::move(handler), permission = std::move(permission), setts = std::move(setts)](TgBot::Message::Ptr msg)
        {
            std::vector<std::string> args = fillArgs(setts, msg->text.value());
            if (args.size() < setts.minArgsCount || args.size() > setts.maxArgsCount)
            {
                this->bot_.getApi().sendMessage(msg->chat->id, "Неверное число аргументов команды");
                return;
            }
            if (!(this->checker_.*permission)(msg->chat->type, msg->chat->id, msg->from->id))
            {
                this->bot_.getApi().sendMessage(msg->chat->id, "Доступ заблокирован");
                return;
            }
            boost::asio::co_spawn(this->ex_, (this->cmdProc_.*handler)(std::move(args), std::move(msg)), boost::asio::detached);
        };
        bot_.getEvents().onCommand(command, lambda);
    }

    void registrateMessageAddress(MessageHandler handler)
    {
        auto lambda = [this, handler = std::move(handler)](TgBot::Message::Ptr msg)
        {
            if (isAddressToMe(msg, this->me_))
                boost::asio::co_spawn(this->ex_, (this->msgProc_.*handler)(std::move(msg)), boost::asio::detached);
        };
        bot_.getEvents().onNonCommandMessage(lambda);
    }
    void registrateMessageInChat(MessageHandler handler)
    {
        auto lambda = [this, handler = std::move(handler)](TgBot::Message::Ptr msg)
        {
            if (!isAddressToMe(msg, this->me_))
                boost::asio::co_spawn(this->ex_, (this->msgProc_.*handler)(std::move(msg)), boost::asio::detached);
        };
        bot_.getEvents().onNonCommandMessage(lambda);
    }

private:
    std::string me_;

    boost::asio::any_io_executor   ex_;
    TgBot::Bot                    &bot_;
    PermissionChecker              checker_;
    middleware::CommandsProcessor &cmdProc_;
    middleware::MessagesProcessor &msgProc_;

private:
    static bool isAddressToMe(TgBot::Message::Ptr msg, const std::string &me)
    {
        if (msg->chat->type == TgBot::Chat::Type::Private)
            return true;
        if (msg->replyToMessage && msg->replyToMessage->from &&
            msg->replyToMessage->from->username.value_or("") == me)
            return true;

        // 3) Упоминание @bot в тексте
        const std::string text = msg->text.value_or("");
        for (const auto &e : msg->entities.value_or({}))
        {
            if (e->type == TgBot::MessageEntity::Type::Mention)
            {
                std::string_view mention(text.begin() + e->offset + 1, text.begin() + e->offset + e->length); // "@my_bot"
                if (mention == me)
                    return true;
            }
        }
        return false;
    }

    static std::vector<std::string> fillArgs(const CommandSettings &setts, const std::string &str)
    {
        const std::size_t spaceIndex = std::min(utils::StringUtils::findSpaceSymbol(str), str.size());
        std::string_view  gluedArgs(str.begin() + spaceIndex, str.end());
        if (!setts.glueArgs)
            return utils::StringUtils::splitArgs<std::string>(gluedArgs);
        return {std::string(gluedArgs)};
    }
};
} // namespace bot