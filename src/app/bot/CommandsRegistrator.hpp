#pragma once

#include <vector>

#include <tgbot/Api.h>

#include "app/config/Commands.hpp"

namespace bot
{
class CommandsRegistrator
{
public:
    static void registerCommands(const TgBot::Api &api, const config::AllCommands &config)
    {
        std::vector<TgBot::BotCommand::Ptr> commands;
        for (const config::Command *ptr = &config.clear; ptr <= &config.remove_chat; ++ptr)
            commands.push_back(makeCommand(*ptr));

        api.setMyCommands(commands);
    }

private:
    static TgBot::BotCommand::Ptr makeCommand(config::Command cmnd)
    {
        auto c = std::make_shared<TgBot::BotCommand>();
        c->command = std::move(cmnd.command);
        c->description = std::move(cmnd.description);
        return c;
    }
};
} // namespace bot