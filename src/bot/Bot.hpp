#pragma once

#include <string>

#include <tgbot/Bot.h>

#include <config/AppConfig.hpp>
#include <openaiprocessor/MessageProcessor.hpp>
#include <tgbot/TgLongPoll.h>
#include <utils/MethodBinder.hpp>

#include "BotMessageSener.hpp"

class Bot
{
public:
    Bot(config::AppConfig config)
        : bot_(std::move(config.token)),
          sender_(bot_.getApi()),
          processor_(sender_)
    {
        initHendlers();
    }

    void run()
    {
        TgBot::TgLongPoll poll(bot_);
        poll.startLoop();
    }

private:
    TgBot::Bot bot_;
    BotMessageSener sender_;
    MessageProcessor processor_;

private:
    void initHendlers()
    {
        bot_.getEvents().onNonCommandMessage(utils::buildMethod(&Bot::onMessage, this));
    }

private:
    void onMessage(std::shared_ptr<TgBot::Message> msg)
    {
        if (msg->text)
            processor_.processMessage(msg->chat->id, msg->text.value());
    }
};