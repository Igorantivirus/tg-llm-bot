#pragma once

#include <string>

#include <config/AppConfig.hpp>
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
    Bot(std::string tgBotApiToken, openai::ChatsProcessor &processor)
        : bot_(std::move(tgBotApiToken)),
          sender_(bot_.getApi()),
          processor_(processor),
          cmd_(*this, bot_, sender_, processor_)
    {
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
    BotMessageSener         sender_;    // Отправитель сообщений
    openai::ChatsProcessor &processor_; // Запросы к OpenAi
    CommandRegistrator<Bot> cmd_;       // Регистратор команд

    std::optional<TgBot::TgLongPoll> poll_; // Для запуска бота

private:
    void initHendlers()
    {
        cmd_.registrate("start", &Bot::onStart);
        cmd_.registrate("clear", &Bot::onClear);
        cmd_.registrate("system", &Bot::onSystem, 0, 1);
        cmd_.registrate("model", &Bot::onModel, 0, 1);
        cmd_.registrate("models", &Bot::onModels);

        bot_.getEvents().onNonCommandMessage(utils::buildMethod(&Bot::onMessage, this));
    }

private:
private: // Команды
    // Старт
    void onStart(const ChatID chatId)
    {
        std::cout << "Start\n";
        sender_.sendMessage(chatId, "Прив");
    }
    // Очистить контекст
    void onClear(const ChatID chatId)
    {
        processor_.clearContext(chatId);
        sender_.sendMessage(chatId, "Контекст отчищен.");
    }
    // Установить системный промт или посмотреть текущий
    void onSystem(const ChatID chatId, ArgsType args)
    {
        if (args.empty())
        {
            auto setts = processor_.getSettings(chatId);
            sender_.sendMessage(chatId, "Системный промт: " + setts.systemPromt);
        }
        else
        {
            processor_.setSystem(chatId, std::string(args[0]));
            sender_.sendMessage(chatId, "Системный промт установлен.");
        }
    }
    // Установить текущую модель или посмотреть текущую
    void onModel(const ChatID chatId, ArgsType args)
    {
        if (args.empty())
        {
            std::string model = processor_.getSettings(chatId).model;
            sender_.sendMessage(chatId, "Текущая модель: " + model);
        }
        else
        {
            std::string model(args[0]);
            processor_.setModelToChat(chatId, model);
            sender_.sendMessage(chatId, "Текущая установлена модель " + model);
        }
    }
    // Вывести список моделей
    void onModels(const ChatID chatId)
    {
        auto        models = processor_.getModels();
        std::string resMsg;
        for (auto &model : models)
        {
            resMsg += std::move(model);
            resMsg.push_back('\n');
        }
        sender_.sendMessage(chatId, std::move(resMsg));
    }

private:
    void onMessage(std::shared_ptr<TgBot::Message> msg)
    {
        if (msg->text)
            processor_.sendMessage(msg->chat->id, msg->text.value());
        else
            processor_.sendMessage(msg->chat->id, "Сори, я сейчас понимаю только текст . . .");
    }
};

} // namespace bot