#pragma once

#include "EventRegistrator.hpp"
#include <app/config/Commands.hpp>
#include <app/handlers/CommandsProcessor.hpp>
#include <app/handlers/MessagesProcessor.hpp>
#include <app/handlers/QueryProcessor.hpp>

namespace bot
{

class BotCustomizer
{
public:
    BotCustomizer(TgBot::Bot &bot, EventRegistrator &cmdReger)
        : bot_(bot), reg_(cmdReger)
    {
    }

    void run()
    {
        poll_.emplace(bot_);
        poll_->startLoop();
    }

    void stop()
    {
        if (poll_)
        {
            poll_->stop();
            poll_ = std::nullopt;
        }
    }

    void initHandlers(config::AllCommands cmnds)
    {
        reg_.registrateCommand(cmnds.clear.command, &handlers::CommandsProcessor::clearCommand, &PermissionChecker::checkBaseCommand, {0, 0});
        reg_.registrateCommand(cmnds.stop.command, &handlers::CommandsProcessor::stopCommand, &PermissionChecker::checkBaseCommand, {0, 0});
        reg_.registrateCommand(cmnds.stop_all.command, &handlers::CommandsProcessor::stopAllCommand, &PermissionChecker::checkBaseCommand, {0, 0});
        reg_.registrateCommand(cmnds.system.command, &handlers::CommandsProcessor::systemCommand, &PermissionChecker::checkBaseCommand, {0, 1, true});
        reg_.registrateCommand(cmnds.models.command, &handlers::CommandsProcessor::modelsCommand, &PermissionChecker::checkBaseCommand, {0, 0});
        reg_.registrateCommand(cmnds.make_admin.command, &handlers::CommandsProcessor::makeAdmin, &PermissionChecker::checkPermissionCommand, {1, 1});
        reg_.registrateCommand(cmnds.remove_admin.command, &handlers::CommandsProcessor::removeAdmin, &PermissionChecker::checkPermissionCommand, {1, 1});
        reg_.registrateCommand(cmnds.ban_user.command, &handlers::CommandsProcessor::banUser, &PermissionChecker::checkPermissionCommand, {1, 1});
        reg_.registrateCommand(cmnds.unban_user.command, &handlers::CommandsProcessor::unbanUser, &PermissionChecker::checkPermissionCommand, {1, 1});
        reg_.registrateCommand(cmnds.add_group.command, &handlers::CommandsProcessor::addGroup, &PermissionChecker::checkPermissionCommand, {0, 1});
        reg_.registrateCommand(cmnds.remove_group.command, &handlers::CommandsProcessor::removeGroup, &PermissionChecker::checkPermissionCommand, {0, 1});
        reg_.registrateCommand(cmnds.add_chat.command, &handlers::CommandsProcessor::addChat, &PermissionChecker::checkPermissionCommand, {0, 1});
        reg_.registrateCommand(cmnds.remove_chat.command, &handlers::CommandsProcessor::removeChat, &PermissionChecker::checkPermissionCommand, {0, 1});

        reg_.registrateMessageInChat(&handlers::MessagesProcessor::addMessage);
        reg_.registrateMessageAddress(&handlers::MessagesProcessor::processMessage);

        reg_.registrateQuery(&handlers::QueryProcessor::onQuery);
    }

private:
    TgBot::Bot       &bot_;
    EventRegistrator &reg_;

    std::optional<TgBot::TgLongPoll> poll_;
};

} // namespace bot