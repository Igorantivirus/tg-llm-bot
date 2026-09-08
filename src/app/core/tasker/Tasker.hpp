#pragma once

#include "openai/dto/ChatCompletions/Request.hpp"
#include <boost/system/detail/error_code.hpp>
#include <openai/ChatsProcessor.hpp>
#include <utils/StreamGenerator.hpp>

#include <app/core/Error.hpp>
#include <app/core/Presentation/InfoType.hpp>
#include <app/core/Presentation/Presenter.hpp>
#include <app/core/tasker/Registration.hpp>
#include <app/core/tasker/StopableGenerator.hpp>

namespace core
{

class Tasker
{
public:
    Tasker(Presenter &presenter, openai::ChatsProcessor &proc)
        : presenter_(presenter), proc_(proc)
    {
    }

    asio::awaitable<void> presentInfo(OperationInfo::Ptr info, InfoType type)
    {
        co_await presenter_.presentInfo(std::move(info), type);
    }

    // Управляющие
    asio::awaitable<void> stop(OperationInfo::Ptr info)
    {
        if (auto found = stopsSignals_.find(info->getChatId()); found != stopsSignals_.end())
        {
            *found->second = true;
            co_await presenter_.presentInfo(std::move(info), InfoType::GenerationStopped);
        }
        co_return;
    }
    asio::awaitable<void> clear(OperationInfo::Ptr info)
    {
        proc_.settings().repo().clearHostory(info->getChatId());
        co_await presenter_.presentInfo(std::move(info), InfoType::ContextCleared);
        co_return;
    }
    // system
    asio::awaitable<void> presentSystem(OperationInfo::Ptr info)
    {
        const auto &history = proc_.settings().repo().getHistoryById(info->getChatId());
        co_await presenter_.presentSystem(info, history.system);
    }
    asio::awaitable<void> setSystem(OperationInfo::Ptr info, std::string system)
    {
        proc_.settings().repo().setSystem(info->getChatId(), std::move(system));
        co_return;
    }
    // models
    asio::awaitable<void> presentModels(OperationInfo::Ptr info)
    {
        co_await presenter_.presentModels(info, proc_.settings().models(), proc_.settings().repo().getHistoryById(info->getChatId()).model);
    }
    asio::awaitable<void> setModel(OperationInfo::Ptr info, std::string model)
    {
        if (!proc_.settings().models().contains(model))
            co_await presenter_.presentInfo(std::move(info), InfoType::ModelNotSetedNoExist);
        else
            proc_.settings().repo().setModel(info->getChatId(), std::move(model));
        co_return;
    }
    // effort
    asio::awaitable<void> presentEfforts(OperationInfo::Ptr info)
    {
        auto                                     values = magic_enum::enum_values<dto::ReasoningEffort>();
        std::unordered_set<dto::ReasoningEffort> efforts = values | std::ranges::to<std::unordered_set<dto::ReasoningEffort>>();
        co_await presenter_.presentEfforts(std::move(info), std::move(efforts), proc_.settings().repo().getHistoryById(info->getChatId()).effort);
        co_return;
    }
    asio::awaitable<void> setEffort(OperationInfo::Ptr info, dto::ReasoningEffort effort)
    {
        proc_.settings().repo().setEffort(info->getChatId(), effort);
        co_return;
    }
    // messages
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
        Registration      reg(stopsSignals_[info->getChatId()]);
        StopableGenerator stopableGen(std::move(gen.value()), reg.stop());
        co_await presenter_.presentMessage(info, stopableGen); // StopableGenerator no movable. поэтому гарантируется, что presentMessage завершится раньше, чем унечтожится Registration
    }

private:
    openai::ChatsProcessor &proc_;

    Presenter &presenter_;

    std::unordered_map<openai::ChatIdType, std::shared_ptr<bool>> stopsSignals_;
};
} // namespace core