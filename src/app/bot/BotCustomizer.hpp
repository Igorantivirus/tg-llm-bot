#pragma once

#include "EventRegistrator.hpp"
#include "app/middleware/MessagesProcessor.hpp"
#include <app/middleware/CommandsProcessor.hpp>

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
    void initHandlers()
    {
        reg_.registrateCommand("clear", &middleware::CommandsProcessor::clearCommand, &PermissionChecker::checkBaseCommand, {0, 0});
        reg_.registrateCommand("stop", &middleware::CommandsProcessor::stopCommand, &PermissionChecker::checkBaseCommand, {0, 0});
        reg_.registrateCommand("system", &middleware::CommandsProcessor::systemCommand, &PermissionChecker::checkBaseCommand, {0, 1, true});
        reg_.registrateCommand("model", &middleware::CommandsProcessor::modelCommand, &PermissionChecker::checkBaseCommand, {1, 1});
        reg_.registrateCommand("models", &middleware::CommandsProcessor::modelsCommand, &PermissionChecker::checkBaseCommand, {0, 0});
        reg_.registrateCommand("make_admin", &middleware::CommandsProcessor::makeAdmin, &PermissionChecker::checkBaseCommand, {0, 1});

        reg_.registrateMessageInChat(&middleware::MessagesProcessor::addMessage);
        reg_.registrateMessageAddress(&middleware::MessagesProcessor::processMessage);
    }

private:
    TgBot::Bot       &bot_;
    EventRegistrator &reg_;

    std::optional<TgBot::TgLongPoll> poll_;
};

} // namespace bot