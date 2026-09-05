#pragma once

#include "app/transport/TgBotMessageSender.hpp"
#include "magic_enum/magic_enum.hpp"
#include "openai/ChatsProcessor.hpp"
#include "presentation/KeyBoardGenerate.hpp"
#include <string>

#include <app/config/Locale.hpp>
#include <app/core/Presentation/Presenter.hpp>
#include <utils/Format.hpp>

namespace transport
{
class TgBotPresentation : public core::Presenter
{
public:
    TgBotPresentation(TgBotMessageSender &sender, openai::ChatsProcessor &proc, config::Locale locale)
        : sender_(sender), proc_(proc), locale_(std::move(locale))
    {
    }

    asio::awaitable<void> presentMessage(core::OperationInfo::Ptr info, utils::StreamGenerator<std::string> &gen) override
    {
        TgBot::Message::Ptr msg = co_await sender_.sendMessage(info->getChatId(), locale_.thinking);
        if (!msg)
            co_return;
        co_await sender_.sendAction(info->getChatId(), transport::ChatAction::typing);
        std::size_t  lastSize = 0;
        std::string  accum;
        std::int64_t msgId = msg->messageId;

        while (auto next = co_await gen.next())
        {
            accum += *next;
            if (accum.size() > lastSize + prSize_)
            {
                lastSize = accum.size();
                std::ignore = co_await sender_.editMessage(info->getChatId(), msgId, accum);
            }
        }
        if (gen.isError())
        {
            accum += '\n' + utils::Format::format(locale_.error, gen.endReason().message());
            std::ignore = co_await sender_.editMessage(info->getChatId(), msgId, accum);
        }
        if (accum.size() != lastSize)
            std::ignore = co_await sender_.editMessage(info->getChatId(), msgId, accum);
        co_return;
    }
    asio::awaitable<void> presentInfo(core::OperationInfo::Ptr info, const core::InfoType msgInfo) override
    {
        std::ignore = co_await sender_.sendMessage(info->getChatId(), utils::Format::format(locale_.info, infoToString(msgInfo)));
        co_return;
    }
    asio::awaitable<void> presentError(core::OperationInfo::Ptr info, const utils::ErrorCode err) override
    {
        std::ignore = co_await sender_.sendMessage(info->getChatId(), utils::Format::format(locale_.error, err.to_string()));
        co_return;
    }

    asio::awaitable<void> presentModels(core::OperationInfo::Ptr info, std::unordered_set<std::string> models) override
    {
        std::string                      msg = utils::Format::format(locale_.currentModel, proc_.settings().repo().getHistoryById(info->getChatId()).model);
        TgBot::InlineKeyboardMarkup::Ptr kb = KeyBoardGenerate::generateForModels(proc_.settings().models(), info->getChatId());
        std::ignore = co_await sender_.sendMessage(info->getChatId(), std::move(msg), std::move(kb));
        co_return;
    }
    asio::awaitable<void> presentModel(core::OperationInfo::Ptr info, std::string model) override
    {
        std::ignore = co_await sender_.sendMessage(info->getChatId(), utils::Format::format(locale_.modelHelp, model));
        co_return;
    }
    asio::awaitable<void> presentSystem(core::OperationInfo::Ptr info, std::string system) override
    {
        std::ignore = co_await sender_.sendMessage(info->getChatId(), utils::Format::format(locale_.systemPromt, system));
        co_return;
    }

private:
    TgBotMessageSender     &sender_;
    openai::ChatsProcessor &proc_;

    config::Locale locale_;
    std::size_t    prSize_ = 200;

private:
    std::string infoToString(const core::InfoType info)
    {
        std::string name(magic_enum::enum_name(info));
        if (auto found = locale_.infoLocale.find(std::string(name)); found != locale_.infoLocale.end())
            return found->second;
        return "Info: " + name;
    }
};
} // namespace transport