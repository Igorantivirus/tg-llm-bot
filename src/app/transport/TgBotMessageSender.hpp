#pragma once

#include <sstream>

#include <boost/asio/awaitable.hpp>
#include <tgbot/Api.h>
#include <tgbot/Types.h>

#include <app/Types.hpp>
#include <app/transport/TgBotApiRedirector.hpp>

namespace transport
{
class TgBotMessageSender
{
public:
    TgBotMessageSender(TgBotApiRedirector &redirector)
        : redirector_(redirector)
    {
    }

    template <typename... Types>
    asio::awaitable<void> sendMessage(const app::ChatId id, Types &&...args)
    {
        std::ostringstream sout;
        ((sout << std::forward<Types>(args)), ...);
        std::string str = sout.str();
        std::ignore = co_await redirector_.call([id, str = std::move(str)](const TgBot::Api &api) -> TgBot::Message::Ptr
        {
            return api.sendMessage(id, str);
        });
        co_return;
    }

private:
    TgBotApiRedirector &redirector_;
};
} // namespace transport