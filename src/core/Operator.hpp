#pragma once

#include <queue>

#include <openai/ChatsProcessor.hpp>
#include <utils/BusyGuard.hpp>

#include "Presentation/Presenter.hpp"
#include "Tasker/Tasker.hpp"

namespace core
{
class Operator
{
public:
    Operator(Presenter &presenter, openai::ChatsProcessor &proc)
        : tasker_(presenter, proc), presenter_(presenter)
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
    //==========================
    // Задачи
    //==========================
    asio::awaitable<void> setSystem(OperationInfo::Ptr info, std::string system)
    {
        processTask(co_await asio::this_coro::executor, info, 0, tasker_.setSystem(info, std::move(system)));
    }
    asio::awaitable<void> getSystem(OperationInfo::Ptr info)
    {
        processTask(co_await asio::this_coro::executor, info, 0, tasker_.getSystem(info));
    }
    asio::awaitable<void> clear(OperationInfo::Ptr info)
    {
        processTask(co_await asio::this_coro::executor, info, 0, tasker_.clear(info));
    }
    asio::awaitable<void> models(OperationInfo::Ptr info)
    {
        processTask(co_await asio::this_coro::executor, info, 0, tasker_.models(info));
    }
    asio::awaitable<void> model(OperationInfo::Ptr info)
    {
        processTask(co_await asio::this_coro::executor, info, 0, tasker_.model(info));
    }
    asio::awaitable<void> setModel(OperationInfo::Ptr info, std::string model)
    {
        processTask(co_await asio::this_coro::executor, info, 0, tasker_.setModel(info, std::move(model)));
    }
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
    Presenter                                                                &presenter_;
    std::unordered_map<openai::ChatIdType, std::queue<asio::awaitable<void>>> queues_;
    bool                                                                      busy_ = false;

private: // Установка и обработки очереди
    inline void processTask(asio::any_io_executor executor, OperationInfo::Ptr info, const std::uint8_t priority, asio::awaitable<void> task)
    {
        if (!queues_[info->getChatId()].empty()) // Если что-то есть, скажем, что поставили в очередь
            // asio::co_spawn(executor, presenter_.presentInfo(info, InfoType::WaitPrevTask), asio::detached); //
            asio::co_spawn(executor, presenter_.presentInfo(info, InfoType::WaitPrevTask), [](std::exception_ptr e)
            {
                if (e)
                    try
                    {
                        std::rethrow_exception(e);
                    }
                    catch (const std::exception &ex)
                    {
                        std::cerr << "COROUTINE: " << ex.what() << '\n';
                    }
                    catch (...)
                    {
                        std::cerr << "Unknown error\n";
                    }
            });
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