#pragma once

#include "openai/dto/Image/ImageEnums.hpp"
#include "openai/messagegenerators/AssistentMessage.hpp"
#include <string>

#include <boost/asio/awaitable.hpp>
#include <magic_enum/magic_enum.hpp>

#include <app/config/Locale.hpp>
#include <app/core/Presentation/Presenter.hpp>
#include <app/transport/TgBotMessageSender.hpp>
#include <app/transport/presentation/ChatAction.hpp>
#include <app/transport/presentation/KeyBoardGenerate.hpp>
#include <app/transport/presentation/MarkdownV2.hpp>
#include <openai/Tools/DefaultTools/CreateImage.hpp>
#include <utils/Format.hpp>

namespace transport
{
class TgBotPresentation : public core::Presenter
{
public:
    TgBotPresentation(TgBotMessageSender &sender, config::Locale locale)
        : sender_(sender), locale_(std::move(locale))
    {
    }

    asio::awaitable<void> presentMessage(core::OperationInfo::Ptr info, utils::StreamGenerator<openai::AssistentMessage> &gen) override
    {
        const app::ChatId chatId = info->getChatId();

        app::MessId   msgId = 0;
        std::string   accum;
        std::size_t   sent = 0;
        MarkdownState state;

        std::ignore = sender_.sendAction(chatId, transport::ChatAction::typing);

        while (auto next = co_await gen.next())
        {
            if (!next->toolCallResult.empty())
                co_await processTools(next->toolCallResult, chatId);
            accum += next->content;
            while (accum.size() > maxMessageSize_)
            {
                const std::size_t cut = cutSize(accum);
                std::string       tail = accum.substr(cut);
                accum.resize(cut);
                co_await finish(chatId, msgId, accum, state);
                msgId = 0;
                accum = std::move(tail);
                sent = 0;
            }
            if (accum.size() >= sent + chunkMessageSize_ && co_await editOrSend(chatId, msgId, accum, false))
                sent = accum.size();
        }

        if (gen.isError())
            accum += '\n' + utils::Format::format(locale_.error, gen.endReason().message());
        if (!accum.empty())
            co_await finish(chatId, msgId, accum, state);
        co_return;
    }
    asio::awaitable<void> presentInfo(core::OperationInfo::Ptr info, const core::InfoType type) override
    {
        std::ignore = co_await sender_.sendMessage(info->getChatId(), utils::Format::format(locale_.info, infoToString(type)));
        co_return;
    }
    asio::awaitable<void> presentError(core::OperationInfo::Ptr info, const utils::ErrorCode err) override
    {
        std::ignore = co_await sender_.sendMessage(info->getChatId(), utils::Format::format(locale_.error, err.to_string()));
        co_return;
    }
    asio::awaitable<void> presentModels(core::OperationInfo::Ptr info, std::unordered_set<std::string> models, std::string curModel) override
    {
        TgBot::InlineKeyboardMarkup::Ptr kb = KeyBoardGenerate::generateForModels(models, curModel);
        std::ignore = co_await sender_.sendMessage(info->getChatId(), "Выберите модель", std::move(kb));
        co_return;
    }
    asio::awaitable<void> presentEfforts(core::OperationInfo::Ptr info, std::unordered_set<dto::ReasoningEffort> efforts, dto::ReasoningEffort curEff) override
    {
        TgBot::InlineKeyboardMarkup::Ptr kb = KeyBoardGenerate::generateForEfforts(efforts, curEff);
        std::ignore = co_await sender_.sendMessage(info->getChatId(), "Выберите effort (на сколько хорошо модель будет думать)", std::move(kb));
        co_return;
    }
    asio::awaitable<void> presentSystem(core::OperationInfo::Ptr info, std::string system) override
    {
        std::ignore = co_await sender_.sendMessage(info->getChatId(), utils::Format::format(locale_.systemPromt, system));
        co_return;
    }

private:
    TgBotMessageSender &sender_;
    config::Locale      locale_;

    std::size_t maxMessageSize_ = 3000;
    std::size_t chunkMessageSize_ = 200;

private:
    asio::awaitable<void> processTools(const std::vector<openai::ToolResult::Ptr> &tools, const app::ChatId chatId)
    {
        for (auto &tool : tools)
        {
            if (tool->calledFunction() == openai::CreateImageToolResult::calledFunctionName)
            {
                openai::CreateImageToolResult *ptr = tool->to<openai::CreateImageToolResult>();
                if (!ptr)
                    continue;
                const auto &dto = ptr->getDto();
                if (!dto.data)
                    continue;
                for (const auto &img : dto.data.value())
                {
                    if (img.b64_json)
                        co_await sender_.sendPhotob64(chatId, img.b64_json.value(), dto.output_format ? dto.output_format.value() : dto::ImageOutputFormat::jpeg);
                }
            }
        }
        co_return;
    }

    asio::awaitable<bool> editOrSend(const app::ChatId chatId, app::MessId &msgId, const std::string &text, const bool md)
    {
        if (msgId != 0)
            co_return static_cast<bool>(co_await sender_.editMessage(chatId, msgId, text, nullptr, md));
        TgBot::Message::Ptr msg = co_await sender_.sendMessage(chatId, text, nullptr, md);
        if (!msg)
            co_return false;
        msgId = msg->messageId;
        co_return true;
    }
    asio::awaitable<void> finish(const app::ChatId chatId, app::MessId msgId, const std::string &text, MarkdownState &state)
    {
        const std::string md = MarkdownV2::convert(text, state);
        if (co_await editOrSend(chatId, msgId, md, true))
            co_return;
        std::ignore = co_await editOrSend(chatId, msgId, text, false);
        co_return;
    }

    std::size_t cutSize(const std::string &text) const
    {
        if (const std::size_t nl = text.rfind('\n', maxMessageSize_); nl != std::string::npos && nl > maxMessageSize_ / 2)
            return nl + 1;

        std::size_t cut = maxMessageSize_;
        // Продолжения многобайтового символа имеют вид 10xxxxxx.
        while (cut > 0 && (static_cast<unsigned char>(text[cut]) & 0xC0) == 0x80)
            --cut;
        return cut;
    }
    std::string infoToString(const core::InfoType info)
    {
        std::string name(magic_enum::enum_name(info));
        if (auto found = locale_.infoLocale.find(std::string(name)); found != locale_.infoLocale.end())
            return found->second;
        return "Info: " + name;
    }
};
} // namespace transport