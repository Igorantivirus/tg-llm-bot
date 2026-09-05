#pragma once

#include "magic_enum/magic_enum.hpp"
#include "presentation/ChatAction.hpp"
#include <sstream>

#include <boost/asio/awaitable.hpp>
#include <string>
#include <tgbot/Api.h>
#include <tgbot/Types.h>

#include <app/Types.hpp>
#include <app/transport/TgBotApiRedirector.hpp>
#include <tuple>

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

    asio::awaitable<void> sendAction(const app::ChatId id, transport::ChatAction act)
    {
        std::ignore = co_await redirector_.call([id, act](const TgBot::Api &api) -> void
        {
            std::string actStr(magic_enum::enum_name(act));
            api.sendChatAction(id, actStr);
        });
        co_return;
    }

    asio::awaitable<TgBot::Message::Ptr> sendMessage(const app::ChatId id, std::string msg, TgBot::InlineKeyboardMarkup::Ptr kb = nullptr, bool md = false)
    {
        auto res = co_await redirector_.call([id, md, msg = std::move(msg), kb = std::move(kb)](const TgBot::Api &api) -> TgBot::Message::Ptr
        {
            return api.sendMessage(id, msg, nullptr, nullptr, kb, md ? "MarkdownV2" : "");
        });
        co_return res ? res.value() : nullptr;
    }

    asio::awaitable<TgBot::Message::Ptr> editMessage(const app::ChatId chatid, const app::MessId msgId, std::string msg)
    {
        auto res = co_await redirector_.call([chatid, msgId, msg = std::move(msg)](const TgBot::Api &api) -> TgBot::Message::Ptr
        {
            return api.editMessageText(std::move(msg), chatid, msgId, "", "", nullptr);
        });
        co_return res ? res.value() : nullptr;
    }

    asio::awaitable<void> answerCallBackQuery(std::string id, std::string msg)
    {
        std::ignore = co_await redirector_.call([id = std::move(id), msg = std::move(msg)](const TgBot::Api &api) -> void
        {
            api.answerCallbackQuery(id, msg, false);
        });
        co_return;
    }

    asio::awaitable<void> sendCommands(std::vector<TgBot::BotCommand::Ptr> commands)
    {
        std::ignore = co_await redirector_.call([commands = std::move(commands)](const TgBot::Api &api) -> void
        {
            api.setMyCommands(std::move(commands));
        });
        co_return;
    }

private:
    TgBotApiRedirector &redirector_;
};
} // namespace transport