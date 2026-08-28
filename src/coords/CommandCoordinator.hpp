#pragma once

#include <openai/ChatsProcessor.hpp>
#include <openai/MessageSender.hpp>

namespace coords
{
class CommandCoordinator
{
public:
    CommandCoordinator(openai::MessageSender &sender, openai::ChatsProcessor &processor)
        : sender_(sender), processor_(processor)
    {
    }

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
    void onSystem(const ChatID chatId, std::vector<std::string> args)
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
    void onModel(const ChatID chatId, std::vector<std::string> args)
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
            sender_.sendMessage(chatId, "Установлена модель " + model);
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
    openai::MessageSender  &sender_;
    openai::ChatsProcessor &processor_;
};
} // namespace coords