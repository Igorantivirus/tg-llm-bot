#pragma once

#include <iostream>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <tgbot/Api.h>
#include <tgbot/TgException.h>
#include <utils/Types.hpp>

#include <app/Error.hpp>

namespace transport
{
class TgBotApiRedirector
{
public:
    TgBotApiRedirector(asio::thread_pool &pool, const TgBot::Api &api)
        : pool_(pool), api_(api)
    {
    }

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
                co_return fail<R>(app::Error::TgApi, e.what());
            }
            catch (const boost::system::system_error &e)
            {
                co_return fail<R>(app::Error::TgSystem, e.what());
            }
            catch (const std::exception &e)
            {
                co_return fail<R>(app::Error::TgUnknownStd, e.what());
            }
            catch (...)
            {
                co_return fail<R>(app::Error::Unknown, "?");
            }
        }, asio::use_awaitable);
    }

private:
    asio::thread_pool &pool_;
    const TgBot::Api  &api_;

private:
    template <typename Return>
    static utils::SyncResult<Return> fail(app::Error e, const char *what)
    {
        std::cerr << "tg api error :" << what << " (code = " << static_cast<int>(e) << ")\n";
        return std::unexpected(e);
    }
};
} // namespace transport