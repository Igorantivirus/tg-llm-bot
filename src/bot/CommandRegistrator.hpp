#pragma once

#include <bot/BotMessageSener.hpp>
#include <openai/ChatsProcessor.hpp>
#include <tgbot/tgbot.h>
#include <utils/StringUtils.hpp>

#include "Permission.hpp"

namespace bot
{
using ArgsType = std::vector<std::string>;

template <class Class>
class CommandRegistrator
{
public:
    CommandRegistrator(Class &object, TgBot::Bot &bot, bot::BotMessageSener &sender, openai::ChatsProcessor &processor)
        : object_(object),
          bot_(bot),
          sender_(sender),
          processor_(processor)
    {
    }

    void registrate(const std::string &command, void (Class::*method)(const ChatID, ArgsType), const std::size_t countArgsMin, const std::size_t countArgsMax, Permission perm = Permission::User)
    {
        bot_.getEvents().onCommand(command, [command, this, method, countArgsMin, countArgsMax, perm](std::shared_ptr<TgBot::Message> msg)
        {
            if (msg->chat->type != TgBot::Chat::Type::Private)
                return;
            auto args = utils::StringUtils::splitArgs<std::string>(msg->text.value());
            if (args.size() < countArgsMin + 1 || args.size() > countArgsMax + 1)
            {
                sender_.sendMessage(msg->chat->id, "Ошибка. Команда \"" + command + "\" Должна принимать от" + std::to_string(countArgsMin) + " до " + std::to_string(countArgsMax) + " аргументов.");
                return;
            }
            // TODO: check admin permissions
            (object_.*method)(msg->chat->id, ArgsType(args.begin() + 1, args.end()));
        });
    }
    void registrate(const std::string &command, void (Class::*method)(const ChatID, ArgsType), const std::size_t countArgs, Permission perm = Permission::User)
    {
        bot_.getEvents().onCommand(command, [command, this, method, countArgs, perm](std::shared_ptr<TgBot::Message> msg)
        {
            if (msg->chat->type != TgBot::Chat::Type::Private)
                return;
            auto args = utils::StringUtils::splitArgs<std::string>(msg->text.value());
            if (args.size() != countArgs + 1)
            {
                sender_.sendMessage(msg->chat->id, "Ошибка. Команда \"" + command + "\" Должна принимать " + std::to_string(countArgs) + " аргументов.");
                return;
            }
            // TODO: check admin permissions
            (object_.*method)(msg->chat->id, ArgsType(args.begin() + 1, args.end()));
        });
    }
    void registrate(const std::string &command, void (Class::*method)(const ChatID), Permission perm = Permission::User)
    {
        bot_.getEvents().onCommand(command, [this, method, perm](std::shared_ptr<TgBot::Message> msg)
        {
            if (msg->chat->type != TgBot::Chat::Type::Private)
                return;
            // TODO: check admin permissions

            (object_.*method)(msg->chat->id);
        });
    }

private:
    Class &object_;

    TgBot::Bot             &bot_;
    bot::BotMessageSener   &sender_;
    openai::ChatsProcessor &processor_;
};
} // namespace bot