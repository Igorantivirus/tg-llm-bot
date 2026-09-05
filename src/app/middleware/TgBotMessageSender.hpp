#pragma once

#include "TgBotApiRedirector.hpp"
#include "Types.hpp"
#include <boost/asio/awaitable.hpp>
#include <sstream>
#include <tgbot/Api.h>
#include <tgbot/Types.h>
namespace middleware
{
class TgBotMessageSender
{
public:
    TgBotMessageSender(TgBotApiRedirector &redirector)
        : redirector_(redirector)
    {
    }

    template <typename... Types>
    asio::awaitable<void> sendMessage(const ChatId id, Types &&...args)
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
} // namespace middleware