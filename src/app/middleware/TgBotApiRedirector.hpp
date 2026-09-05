#pragma once

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <iostream>
#include <tgbot/Api.h>
#include <tgbot/TgException.h>
#include <utils/Types.hpp>

#include "Error.hpp"

namespace middleware
{
class TgBotApiRedirector
{
public:
    TgBotApiRedirector(asio::thread_pool &pool, const TgBot::Api &api)
        : pool_(pool), api_(api)
    {
    }

    // template <typename Return, typename... MethodArgs, typename... CallArgs>
    //     requires std::invocable<Return (TgBot::Api::*)(MethodArgs...) const, const TgBot::Api &, CallArgs...>
    // utils::AsyncResult<Return> call(Return (TgBot::Api::*method)(MethodArgs...) const, CallArgs... args)
    // {
    //     co_return (co_await asio::co_spawn(pool_, [this, method, ... args = std::move(args)]() mutable -> utils::AsyncResult<Return>
    //     {
    //         try
    //         {
    //             if constexpr (std::is_void_v<Return>)
    //             {
    //                 (api_.*method)(std::move(args)...);
    //                 co_return utils::empty;
    //             }
    //             else
    //                 co_return (api_.*method)(std::move(args)...);
    //         }
    //         catch (const TgBot::TgException &e)
    //         {
    //             co_return fail<Return>(Error::TgApi, e.what());
    //         }
    //         catch (const boost::system::system_error &e)
    //         {
    //             co_return fail<Return>(Error::TgSystem, e.what());
    //         }
    //         catch (const std::exception &e)
    //         {
    //             co_return fail<Return>(Error::TgUnknownStd, e.what());
    //         }
    //         catch (...)
    //         {
    //             co_return fail<Return>(Error::Unknown, "?");
    //         }
    //     }, asio::use_awaitable));
    // }

    template <typename F>
        requires std::invocable<F &, const TgBot::Api &>
    [[nodiscard]] auto call(F f) -> utils::AsyncResult<std::invoke_result_t<F &, const TgBot::Api &>>
    {
        using R = std::invoke_result_t<F &, const TgBot::Api &>;
        co_return co_await asio::co_spawn(pool_, [this, f = std::move(f)]() mutable -> utils::AsyncResult<R>
        {
            try
            {
                if constexpr (std::is_void_v<R>)
                {
                    f(api_);
                    co_return utils::SyncResult<void>{};
                }
                else
                {
                    co_return f(api_);
                }
            }
            catch (const TgBot::TgException &e)
            {
                co_return fail<R>(Error::TgApi, e.what());
            }
            catch (const boost::system::system_error &e)
            {
                co_return fail<R>(Error::TgSystem, e.what());
            }
            catch (const std::exception &e)
            {
                co_return fail<R>(Error::TgUnknownStd, e.what());
            }
            catch (...)
            {
                co_return fail<R>(Error::Unknown, "?");
            }
        }, asio::use_awaitable);
    }

private:
    asio::thread_pool &pool_;
    const TgBot::Api        &api_;

private:
    template <typename Return>
    static utils::SyncResult<Return> fail(Error e, const char *what)
    {
        std::cerr << "tg api error :" << what << " (code = " << static_cast<int>(e) << ")\n";
        return std::unexpected(e);
    }
};
} // namespace middleware