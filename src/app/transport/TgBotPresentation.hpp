#pragma once

#include "app/transport/TgBotMessageSender.hpp"
#include "openai/ChatsProcessor.hpp"
#include <string>

#include <app/core/Presentation/Presenter.hpp>

namespace transport
{
class TgBotPresentation : public core::Presenter
{
public:
    TgBotPresentation(TgBotMessageSender &sender, openai::ChatsProcessor &proc)
        : sender_(sender), proc_(proc)
    {
    }

    asio::awaitable<void> presentMessage(core::OperationInfo::Ptr info, utils::StreamGenerator<std::string> &gen) override
    {
        TgBot::Message::Ptr msg = co_await sender_.sendMessage(info->getChatId(), std::string("Думаю . . ."));
        if (!msg)
            co_return;
        std::size_t  lastSize = 0;
        std::string  accum;
        std::int64_t msgId = msg->messageId;

        while (auto next = co_await gen.next())
        {
            accum += *next;
            if (accum.size() > lastSize + prSize_)
            {
                lastSize = accum.size();
                std::ignore = co_await sender_.editMessage(info->getChatId(), msgId, accum);
            }
        }
        if (gen.isError())
        {
            accum += "\nError: " + gen.endReason().message();
            std::ignore = co_await sender_.editMessage(info->getChatId(), msgId, accum);
        }
        if (accum.size() != lastSize)
            std::ignore = co_await sender_.editMessage(info->getChatId(), msgId, accum);
        co_return;
    }
    asio::awaitable<void> presentInfo(core::OperationInfo::Ptr info, const core::InfoType msgInfo) override
    {
        std::ignore = co_await sender_.sendMessage(info->getChatId(), "Info: " + std::to_string(static_cast<int>(msgInfo)));
        co_return;
    }
    asio::awaitable<void> presentError(core::OperationInfo::Ptr info, const utils::ErrorCode err) override
    {
        std::ignore = co_await sender_.sendMessage(info->getChatId(), "Error: " + err.to_string());
        co_return;
    }

    asio::awaitable<void> presentModels(core::OperationInfo::Ptr info, std::unordered_set<std::string> models) override
    {
        std::string msg = "Текущая модель: " + proc_.settings().repo().getHistoryById(info->getChatId()).model;

        std::ignore = co_await sender_.sendMessage(info->getChatId(), std::move(msg));
        co_return;
    }
    asio::awaitable<void> presentModel(core::OperationInfo::Ptr info, std::string model) override
    {
        std::ignore = co_await sender_.sendMessage(info->getChatId(), "Current mode: " + model);
        co_return;
    }
    asio::awaitable<void> presentSystem(core::OperationInfo::Ptr info, std::string system) override
    {
        std::ignore = co_await sender_.sendMessage(info->getChatId(), "Системный промт: \"" + system + "\"\nЧтоб установить новый системный промт - введите команду \\system. Чтоб очистить системный промт, введите \\system -");
        co_return;
    }

private:
    TgBotMessageSender     &sender_;
    openai::ChatsProcessor &proc_;

    std::size_t prSize_ = 200;
};
} // namespace transport