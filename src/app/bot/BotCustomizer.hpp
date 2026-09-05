#pragma once

#include "EventRegistrator.hpp"
#include <app/handlers/CommandsProcessor.hpp>
#include <app/handlers/MessagesProcessor.hpp>

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
        reg_.registrateCommand("clear", &handlers::CommandsProcessor::clearCommand, &PermissionChecker::checkBaseCommand, {0, 0});
        reg_.registrateCommand("stop", &handlers::CommandsProcessor::stopCommand, &PermissionChecker::checkBaseCommand, {0, 0});
        reg_.registrateCommand("system", &handlers::CommandsProcessor::systemCommand, &PermissionChecker::checkBaseCommand, {0, 1, true});
        reg_.registrateCommand("model", &handlers::CommandsProcessor::modelCommand, &PermissionChecker::checkBaseCommand, {1, 1});
        reg_.registrateCommand("models", &handlers::CommandsProcessor::modelsCommand, &PermissionChecker::checkBaseCommand, {0, 0});
        reg_.registrateCommand("make_admin", &handlers::CommandsProcessor::makeAdmin, &PermissionChecker::checkBaseCommand, {0, 1});

        reg_.registrateMessageInChat(&handlers::MessagesProcessor::addMessage);
        reg_.registrateMessageAddress(&handlers::MessagesProcessor::processMessage);
    }

private:
    TgBot::Bot       &bot_;
    EventRegistrator &reg_;

    std::optional<TgBot::TgLongPoll> poll_;
};

} // namespace bot