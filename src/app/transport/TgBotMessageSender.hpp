#pragma once

#include "utils/Base64.hpp"
#include <string>
#include <tgbot/Api.h>
#include <tgbot/InputFile.h>
#include <tgbot/Types.h>
#include <tuple>

#include <boost/asio/awaitable.hpp>
#include <magic_enum/magic_enum.hpp>

#include <app/Types.hpp>
#include <app/transport/TgBotApiRedirector.hpp>
#include <app/transport/presentation/ChatAction.hpp>
#include <openai/dto/Image/ImageEnums.hpp>

namespace transport
{
class TgBotMessageSender
{
public:
    TgBotMessageSender(TgBotApiRedirector &redirector)
        : redirector_(redirector)
    {
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

    asio::awaitable<TgBot::Message::Ptr> editMessage(const app::ChatId chatid, const app::MessId msgId, std::string msg, TgBot::InlineKeyboardMarkup::Ptr kb = nullptr, bool md = false)
    {
        auto res = co_await redirector_.call([chatid, msgId, md, msg = std::move(msg), kb = std::move(kb)](const TgBot::Api &api) -> TgBot::Message::Ptr
        {
            return api.editMessageText(msg, chatid, msgId, "", md ? "MarkdownV2" : "", nullptr, kb);
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

    asio::awaitable<void> deleteMessage(const app::ChatId chat, const app::MessId mess)
    {
        std::ignore = co_await redirector_.call([chat, mess](const TgBot::Api &api) -> void
        {
            api.deleteMessage(chat, mess);
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
    asio::awaitable<void> sendPhotob64(app::ChatId id, const std::string &base64Photo, dto::ImageOutputFormat format)
    {
        auto res = co_await redirector_.call([id, base64Photo = &base64Photo, format](const TgBot::Api &api) -> void
        {
            auto bytes = utils::Base64::decode(*base64Photo);
            if (!bytes)
                return;
            std::string mimeType = (format == dto::ImageOutputFormat::png) ? "image/png" : ((format == dto::ImageOutputFormat::webp) ? "image/webp" : "image/jpeg");

            auto inputFile = std::make_shared<TgBot::InputFile>();
            inputFile->data = std::move(bytes.value());
            inputFile->mimeType = mimeType;
            inputFile->fileName = "image." + std::string(magic_enum::enum_name(format));

            api.sendPhoto(id, std::move(inputFile));
        });
    }
    asio::awaitable<TgBot::File::Ptr> getFile(std::string fileId)
    {
        auto res = co_await redirector_.call([fileId = std::move(fileId)](const TgBot::Api &api) -> TgBot::File::Ptr
        {
            return api.getFile(fileId);
        });
        if (res)
            co_return res.value();
        co_return nullptr;
    }
    asio::awaitable<std::string> downloadFile(std::string filePath)
    {
        auto res = co_await redirector_.call([filePath = std::move(filePath)](const TgBot::Api &api) -> std::string
        {
            return api.downloadFile(filePath);
        });
        if (res)
            co_return res.value();
        co_return "";
    }

private:
    TgBotApiRedirector &redirector_;
};
} // namespace transport