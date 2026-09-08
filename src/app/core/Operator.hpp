#pragma once

#include "openai/dto/ChatCompletions/Request.hpp"
#include <boost/asio/detached.hpp>
#include <queue>

#include <openai/ChatsProcessor.hpp>
#include <utils/BusyGuard.hpp>

#include <app/core/Presentation/Presenter.hpp>
#include <app/core/Tasker/Tasker.hpp>

namespace core
{
class Operator
{
public:
    Operator(Presenter &presenter, openai::ChatsProcessor &proc)
        : tasker_(presenter, proc)
    {
    }
    //==========================
    // Управляющие вызовы
    //==========================
    asio::awaitable<void> stop(OperationInfo::Ptr info)
    {
        co_await tasker_.stop(info);
    }
    asio::awaitable<void> stopAll(OperationInfo::Ptr info)
    {
        co_await tasker_.stop(info);
        queues_[info->getChatId()] = std::queue<asio::awaitable<void>>{};
    }
    asio::awaitable<void> clear(OperationInfo::Ptr info)
    {
        processTask(co_await asio::this_coro::executor, info, 0, tasker_.clear(info));
    }
    //==========================
    // Задачи
    //==========================
    asio::awaitable<void> presentSystem(OperationInfo::Ptr info)
    {
        processTask(co_await asio::this_coro::executor, info, 0, tasker_.presentSystem(info));
    }
    asio::awaitable<void> setSystem(OperationInfo::Ptr info, std::string system)
    {
        processTask(co_await asio::this_coro::executor, info, 0, tasker_.setSystem(info, std::move(system)));
    }
    asio::awaitable<void> presentModels(OperationInfo::Ptr info)
    {
        processTask(co_await asio::this_coro::executor, info, 0, tasker_.presentModels(info));
    }
    asio::awaitable<void> setModel(OperationInfo::Ptr info, std::string model)
    {
        processTask(co_await asio::this_coro::executor, info, 0, tasker_.setModel(info, std::move(model)));
    }
    asio::awaitable<void> presentEfforts(OperationInfo::Ptr info)
    {
        processTask(co_await asio::this_coro::executor, info, 0, tasker_.presentEfforts(info));
    }
    asio::awaitable<void> setEffort(OperationInfo::Ptr info, dto::ReasoningEffort effort)
    {
        processTask(co_await asio::this_coro::executor, info, 0, tasker_.setEffort(info, std::move(effort)));
    }
    //==========================
    // Сообщения
    //==========================
    asio::awaitable<void> addMessage(OperationInfo::Ptr info, std::string msg, openai::AdditionalsToMessage adds = {})
    {
        processTask(co_await asio::this_coro::executor, info, 0, tasker_.addMessage(info, std::move(msg), std::move(adds)));
    }
    asio::awaitable<void> processMessage(OperationInfo::Ptr info, std::string msg, openai::AdditionalsToMessage adds = {})
    {
        processTask(co_await asio::this_coro::executor, info, 0, tasker_.processMessage(info, std::move(msg), std::move(adds)));
    }

private:
    Tasker                                                                    tasker_;
    std::unordered_map<openai::ChatIdType, std::queue<asio::awaitable<void>>> queues_;
    bool                                                                      busy_ = false;

private: // Установка и обработки очереди
    inline void processTask(asio::any_io_executor executor, OperationInfo::Ptr info, const std::uint8_t priority, asio::awaitable<void> task)
    {
        if (!queues_[info->getChatId()].empty()) // Если что-то есть, скажем, что поставили в очередь
            asio::co_spawn(executor, tasker_.presentInfo(info, InfoType::WaitPrevTask), asio::detached);
        queues_[info->getChatId()].push(std::move(task));             // Добавим операцию в очередь
        asio::co_spawn(executor, processQueue(info), asio::detached); // Запусим очередь
    }

    asio::awaitable<void> processQueue(OperationInfo::Ptr info)
    {
        if (busy_)
            co_return;
        utils::BusyGuard                   bg(busy_);
        std::queue<asio::awaitable<void>> &queue = queues_[info->getChatId()];
        while (!queue.empty())
        {
            asio::awaitable<void> call = std::move(queue.front());
            queue.pop();
            co_await std::move(call);
        }
    }
};
} // namespace core