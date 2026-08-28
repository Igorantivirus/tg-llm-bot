#pragma once

#include <string>

#include <config/AppConfig.hpp>
#include <coords/CommandCoordinator.hpp>
#include <coords/MessageCoordinator.hpp>
#include <openai/ChatsProcessor.hpp>
#include <openai/Types.hpp>
#include <tgbot/Bot.h>
#include <tgbot/TgLongPoll.h>
#include <utils/MethodBinder.hpp>
#include <utils/StringUtils.hpp>

#include "BotMessageSener.hpp"
#include "CommandRegistrator.hpp"

namespace bot
{

class Bot
{
public:
    Bot(
        std::string                 tgBotApiToken,
        openai::ChatsProcessor     &processor,
        BotMessageSener            &sender,
        coords::CommandCoordinator &cmdCoorder,
        coords::MessageCoordinator &msgCoorder)
        : bot_(std::move(tgBotApiToken)),
          sender_(sender),
          processor_(processor),
          cmdCoorder_(cmdCoorder),
          msgCoorder_(msgCoorder),
          cmd_(cmdCoorder_, bot_, sender_, processor_)
    {
        sender_.initTgBotApi(bot_.getApi());
        processor_.setSender(sender_);
        initHendlers();
    }

    void run()
    {
        poll_.emplace(bot_);
        poll_->startLoop();
    }

    void stop()
    {
        if (poll_)
            poll_->stop();
    }

private:
    TgBot::Bot              bot_;       // Телеграмм бот
    BotMessageSener        &sender_;    // Отправитель сообщений
    openai::ChatsProcessor &processor_; // Запросы к OpenAi

    coords::CommandCoordinator &cmdCoorder_;
    coords::MessageCoordinator &msgCoorder_;

    CommandRegistrator<coords::CommandCoordinator> cmd_;  // Регистратор команд
    std::optional<TgBot::TgLongPoll>               poll_; // Для запуска бота

private:
    void initHendlers()
    {
        cmd_.registrate("start", &coords::CommandCoordinator::onStart);
        cmd_.registrate("clear", &coords::CommandCoordinator::onClear);
        cmd_.registrate("system", &coords::CommandCoordinator::onSystem, 0, 1);
        cmd_.registrate("model", &coords::CommandCoordinator::onModel, 0, 1);
        cmd_.registrate("models", &coords::CommandCoordinator::onModels);

        bot_.getEvents().onNonCommandMessage(utils::buildMethod(&coords::MessageCoordinator::onMessage, msgCoorder_));
    }
};

} // namespace bot