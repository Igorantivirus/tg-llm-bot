#pragma once

#include <string>

#include <tgbot/Bot.h>

#include <config/AppConfig.hpp>
#include <openai/ChatsProcessor.hpp>
#include <tgbot/TgLongPoll.h>
#include <utils/MethodBinder.hpp>
#include <utils/StringUtils.hpp>

#include "BotMessageSener.hpp"

namespace bot
{

class Bot
{
public:
    Bot(std::string tgBotApiToken, openai::ChatsProcessor &processor)
        : bot_(std::move(tgBotApiToken)),
          sender_(bot_.getApi()),
          processor_(processor)
    {
        processor_.setSender(sender_);
        initHendlers();
    }

    void run()
    {
        TgBot::TgLongPoll poll(bot_);
        poll.startLoop();
    }

private:
    TgBot::Bot              bot_;
    BotMessageSener         sender_;
    openai::ChatsProcessor &processor_;

private:
    void initHendlers()
    {
        bot_.getEvents().onCommand("start", utils::buildMethod(&Bot::onStart, this));
        bot_.getEvents().onCommand("models", utils::buildMethod(&Bot::onModels, this));
        bot_.getEvents().onCommand("model", utils::buildMethod(&Bot::onModel, this));
        bot_.getEvents().onCommand("currentmodel", utils::buildMethod(&Bot::onCurrentModel, this));
        bot_.getEvents().onNonCommandMessage(utils::buildMethod(&Bot::onMessage, this));
    }

private:
    void onStart(std::shared_ptr<TgBot::Message> msg)
    {
        sender_.sendMessage(msg->chat->id, "Прив");
    }

    void onModels(std::shared_ptr<TgBot::Message> msg)
    {
        auto        models = processor_.getModels();
        std::string resMsg;
        for (auto &model : models)
        {
            resMsg += std::move(model);
            resMsg.push_back('\n');
        }
        sender_.sendMessage(msg->chat->id, std::move(resMsg));
    }

    void onModel(std::shared_ptr<TgBot::Message> msg)
    {
        auto args = utils::StringUtils::splitArgs(msg->text.value());
        if (args.size() != 2)
            sender_.sendMessage(msg->chat->id, "Error\n");

        processor_.setModelToChat(msg->chat->id, std::string(args[1]));
    }

    void onCurrentModel(std::shared_ptr<TgBot::Message> msg)
    {
        std::string model = processor_.getSettings(msg->chat->id).model;
        sender_.sendMessage(msg->chat->id, "Current model: " + model);
    }

    void onMessage(std::shared_ptr<TgBot::Message> msg)
    {
        if (msg->text)
            processor_.sendMessage(msg->chat->id, msg->text.value());
    }
};

} // namespace bot