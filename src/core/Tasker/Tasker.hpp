#pragma once

#include <openai/ChatsProcessor.hpp>
#include <utils/StreamGenerator.hpp>

#include "Registration.hpp"
#include "StopableGenerator.hpp"
#include <core/Presentation/Presenter.hpp>

namespace core
{

class Tasker
{
public:
    Tasker(Presenter &presenter, openai::ChatsProcessor &proc)
        : presenter_(presenter), proc_(proc)
    {
    }

    asio::awaitable<void> stop(OperationInfo::Ptr info)
    {
        if (auto found = stopsSignals_.find(info->getChatId()); found != stopsSignals_.end())
            *found->second = true;
        co_return;
    }

    asio::awaitable<void> setSystem(OperationInfo::Ptr info, std::string system)
    {
        proc_.settings().repo().setSystem(info->getChatId(), std::move(system));
        co_return;
    }

    asio::awaitable<void> getSystem(OperationInfo::Ptr info)
    {
        const auto &history = proc_.settings().repo().getHistoryById(info->getChatId());
        co_await presenter_.presentSystem(info, history.system);
    }

    asio::awaitable<void> clear(OperationInfo::Ptr info)
    {
        proc_.settings().repo().clearHostory(info->getChatId());
        co_return;
    }

    asio::awaitable<void> models(OperationInfo::Ptr info)
    {
        const auto &models = proc_.settings().models();
        co_await presenter_.presentModels(info, models);
    }

    asio::awaitable<void> model(OperationInfo::Ptr info)
    {
        co_await presenter_.presentModel(info, proc_.settings().repo().getHistoryById(info->getChatId()).model);
    }

    asio::awaitable<void> setModel(OperationInfo::Ptr info, std::string model)
    {
        proc_.settings().repo().setModel(info->getChatId(), std::move(model));
        co_return;
    }

    asio::awaitable<void> addMessage(OperationInfo::Ptr info, std::string msg, openai::AdditionalsToMessage adds)
    {
        dto::Message dto = openai::HistoryUtils::constructStartMessage(std::move(msg), std::move(adds));
        proc_.settings().repo().addDialogFragment(info->getChatId(), {std::move(dto)});
        co_return;
    }

    asio::awaitable<void> processMessage(OperationInfo::Ptr info, std::string msg, openai::AdditionalsToMessage adds)
    {
        auto gen = co_await proc_.chatCompletions(info->getChatId(), std::move(msg), std::move(adds));
        if (!gen)
            co_return co_await presenter_.presentError(info, gen.error());
        Registration      reg(stopsSignals_, info->getChatId());
        StopableGenerator stopableGen(std::move(gen.value()), reg.stop());
        co_await presenter_.presentMessage(info, stopableGen);
    }

private:
    openai::ChatsProcessor &proc_;

    Presenter &presenter_;

    std::unordered_map<openai::ChatIdType, bool *> stopsSignals_;
};
} // namespace core