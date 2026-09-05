#pragma once

#include <algorithm>
#include <core/Presentation/Presenter.hpp>
#include <string>
#include <tgbot/Types.h>

#include "TgBotApiRedirector.hpp"

namespace middleware
{
class TgBotPresentation : public core::Presenter
{
public:
    TgBotPresentation(TgBotApiRedirector &redirector)
        : redirector_(redirector)
    {
    }

    asio::awaitable<void> presentMessage(core::OperationInfo::Ptr info, utils::StreamGenerator<std::string> &gen) override
    {
        std::cout << "Chat id: " << info->getChatId() << '\n';
        auto res = co_await redirector_.call([id = info->getChatId()](const TgBot::Api &api) -> TgBot::Message::Ptr
        {
            return api.sendMessage(id, "Думаю . . .");
        });
        if (!res)
            co_return;
        TgBot::Message::Ptr msg = res.value();
        std::size_t         lastSize = 0;
        std::string         accum;
        std::int64_t        msgId = msg->messageId;

        while (auto next = co_await gen.next())
        {
            accum += *next;
            if (accum.size() > lastSize + prSize_)
            {
                lastSize = accum.size();
                std::ignore = co_await redirector_.call([id = info->getChatId(), msgId = msgId, msg = &accum](const TgBot::Api &api) -> TgBot::Message::Ptr
                {
                    return api.editMessageText(*msg, id, msgId);
                });
            }
        }
        if (gen.isError())
        {
            accum += "\nError: " + gen.endReason().message();
            std::ignore = co_await redirector_.call([id = info->getChatId(), msgId = msgId, msg = &accum](const TgBot::Api &api) -> TgBot::Message::Ptr
            {
                return api.editMessageText(*msg, id, msgId);
            });
        }
        if (accum.size() != lastSize)
            std::ignore = co_await redirector_.call([id = info->getChatId(), msgId = msgId, msg = &accum](const TgBot::Api &api) -> TgBot::Message::Ptr
            {
                return api.editMessageText(*msg, id, msgId);
            });
        co_return;
    }
    asio::awaitable<void> presentInfo(core::OperationInfo::Ptr info, const core::InfoType msgInfo) override
    {
        std::ignore = co_await redirector_.call([id = info->getChatId(), msgInfo = msgInfo](const TgBot::Api &api) -> TgBot::Message::Ptr
        {
            return api.sendMessage(id, "Info: " + std::to_string(static_cast<int>(msgInfo)));
        });
        co_return;
    }
    asio::awaitable<void> presentError(core::OperationInfo::Ptr info, const utils::ErrorCode err) override
    {
        std::ignore = co_await redirector_.call([id = info->getChatId(), err = err.to_string()](const TgBot::Api &api) -> TgBot::Message::Ptr
        {
            return api.sendMessage(id, "Error: " + err);
        });
        co_return;
    }

    asio::awaitable<void> presentModels(core::OperationInfo::Ptr info, std::unordered_set<std::string> models) override
    {
        std::string modelsFull;
        std::for_each_n(models.begin(), models.size(), [&modelsFull](const std::string &str)
        {
            modelsFull += str + '\n';
        });
        std::ignore = co_await redirector_.call([id = info->getChatId(), modelsFull = std::move(modelsFull)](const TgBot::Api &api) -> TgBot::Message::Ptr
        {
            return api.sendMessage(id, "Models: " + modelsFull);
        });
        co_return;
    }
    asio::awaitable<void> presentModel(core::OperationInfo::Ptr info, std::string model) override
    {
        std::ignore = co_await redirector_.call([id = info->getChatId(), model = std::move(model)](const TgBot::Api &api) -> TgBot::Message::Ptr
        {
            return api.sendMessage(id, "Current mode: " + model);
        });
        co_return;
    }
    asio::awaitable<void> presentSystem(core::OperationInfo::Ptr info, std::string system) override
    {
        std::ignore = co_await redirector_.call([id = info->getChatId(), system = std::move(system)](const TgBot::Api &api) -> TgBot::Message::Ptr
        {
            return api.sendMessage(id, "Current system: " + system);
        });
        co_return;
    }

private:
    TgBotApiRedirector &redirector_;

    std::size_t prSize_ = 100;
};
} // namespace middleware