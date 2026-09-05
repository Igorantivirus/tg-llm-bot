#pragma once

#include <cstdint>
#include <unordered_map>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/awaitable.hpp>
#include <tgbot/Types.h>

#include <app/handlers/CommandsProcessor.hpp>
#include <app/handlers/MessagesProcessor.hpp>
#include <app/handlers/QueryProcessor.hpp>

#include <app/bot/PermissionChecker.hpp>
#include <app/config/Locale.hpp>
#include <utils/Format.hpp>

namespace bot
{
class EventRegistrator
{
public:
    using CommandHandler = boost::asio::awaitable<void> (handlers::CommandsProcessor::*)(std::vector<std::string>, TgBot::Message::Ptr);
    using MessageHandler = boost::asio::awaitable<void> (handlers::MessagesProcessor::*)(TgBot::Message::Ptr);
    using QueryHandler = boost::asio::awaitable<void>   (handlers::QueryProcessor::*)(transport::Operation, TgBot::Message::Ptr, TgBot::CallbackQuery::Ptr);
    using Permission = bool                             (PermissionChecker::*)(const TgBot::Chat::Type, const app::ChatId, const app::UserId);

    struct CommandSettings
    {
        std::uint8_t minArgsCount = 0;
        std::uint8_t maxArgsCount = 255;
        bool         glueArgs = false;
    };

public:
    EventRegistrator(boost::asio::any_io_executor ex, TgBot::Bot &bot, PermissionChecker checker, handlers::CommandsProcessor &cmdProc, handlers::MessagesProcessor &msgProc, handlers::QueryProcessor &queProc, config::Locale locale)
        : ex_(ex), bot_(bot), checker_(checker), cmdProc_(cmdProc), msgProc_(msgProc), queProc_(queProc), locale_(std::move(locale))
    {
        me_ = bot_.getApi().getMe()->username.value_or("");
    }

    void registrateCommand(const std::string &command, CommandHandler handler, Permission permission, CommandSettings setts)
    {
        auto lambda = [this, handler = std::move(handler), permission = std::move(permission), setts = std::move(setts)](TgBot::Message::Ptr msg)
        {
            std::vector<std::string> args = fillArgs(setts, msg->text.value());
            if (args.size() < setts.minArgsCount || args.size() > setts.maxArgsCount)
                return this->bot_.getApi().sendMessage(msg->chat->id, this->locale_.commandErrors.argumentsError), void();
            if (!(this->checker_.*permission)(msg->chat->type, msg->chat->id, msg->from->id))
                return this->bot_.getApi().sendMessage(msg->chat->id, this->locale_.commandErrors.permissionError), void();
            boost::asio::co_spawn(this->ex_, (this->cmdProc_.*handler)(std::move(args), std::move(msg)), boost::asio::detached);
        };
        bot_.getEvents().onCommand(command, lambda);
    }

    void registrateQuery(transport::OperationType oper, QueryHandler handler, Permission permission)
    {
        queryPermissionByType_[oper] = std::make_pair(std::move(permission), std::move(handler));
        if (setedQueryCallBack_)
            return;
        auto lambda = [this](TgBot::CallbackQuery::Ptr query)
        {
            TgBot::Message::Ptr msg;
            if (query->message)
                if (auto *p = std::get_if<TgBot::Message::Ptr>(&query->message->value))
                    msg = *p;
            if (!msg)
            {
                this->bot_.getApi().answerCallbackQuery(query->id, this->locale_.commandErrors.commandError);
                return;
            }
            auto dto = utils::deserialize<transport::Operation>(query->data.value());
            if (!dto)
                return this->bot_.getApi().answerCallbackQuery(query->id, utils::Format::format(this->locale_.error, dto.error().message())), void();

            auto found = this->queryPermissionByType_.find(dto->type);

            if (found == this->queryPermissionByType_.end())
                return this->bot_.getApi().answerCallbackQuery(query->id, this->locale_.commandErrors.commandError), void();
            else if (!(this->checker_.*found->second.first)(msg->chat->type, msg->chat->id, query->from->id))
                return this->bot_.getApi().answerCallbackQuery(query->id, this->locale_.commandErrors.permissionError), void();

            asio::co_spawn(this->ex_, (this->queProc_.*found->second.second)(std::move(dto.value()), std::move(msg), std::move(query)), boost::asio::detached);
        };
        bot_.getEvents().onCallbackQuery(lambda);
        setedQueryCallBack_ = true;
    }

    void registrateMessageAddress(MessageHandler handler)
    {
        auto lambda = [this, handler = std::move(handler)](TgBot::Message::Ptr msg)
        {
            if (isAddressToMe(msg, this->me_))
                boost::asio::co_spawn(this->ex_, (this->msgProc_.*handler)(std::move(msg)), boost::asio::detached);
        };
        bot_.getEvents().onNonCommandMessage(lambda);
    }
    void registrateMessageInChat(MessageHandler handler)
    {
        auto lambda = [this, handler = std::move(handler)](TgBot::Message::Ptr msg)
        {
            if (!isAddressToMe(msg, this->me_))
                boost::asio::co_spawn(this->ex_, (this->msgProc_.*handler)(std::move(msg)), boost::asio::detached);
        };
        bot_.getEvents().onNonCommandMessage(lambda);
    }

private:
    std::string me_;

    boost::asio::any_io_executor ex_;
    TgBot::Bot                  &bot_;
    PermissionChecker            checker_;
    handlers::CommandsProcessor &cmdProc_;
    handlers::MessagesProcessor &msgProc_;
    handlers::QueryProcessor    &queProc_;

    std::unordered_map<transport::OperationType, std::pair<Permission, QueryHandler>> queryPermissionByType_;
    bool                                                                              setedQueryCallBack_ = false;

    config::Locale locale_;

private:
    static bool isAddressToMe(TgBot::Message::Ptr msg, const std::string &me)
    {
        if (msg->chat->type == TgBot::Chat::Type::Private)
            return true;
        if (msg->replyToMessage && msg->replyToMessage->from &&
            msg->replyToMessage->from->username.value_or("") == me)
            return true;

        // 3) Упоминание @bot в тексте
        const std::string text = msg->text.value_or("");
        for (const auto &e : msg->entities.value_or({}))
        {
            if (e->type == TgBot::MessageEntity::Type::Mention)
            {
                std::string_view mention(text.begin() + e->offset + 1, text.begin() + e->offset + e->length); // "@my_bot"
                if (mention == me)
                    return true;
            }
        }
        return false;
    }

    static std::vector<std::string> fillArgs(const CommandSettings &setts, const std::string &str)
    {
        const std::size_t spaceIndex = std::min(utils::StringUtils::findSpaceSymbol(str), str.size());
        std::string_view  gluedArgs(str.begin() + spaceIndex, str.end());
        if (!setts.glueArgs)
            return utils::StringUtils::splitArgs<std::string>(gluedArgs);
        return {std::string(gluedArgs)};
    }
};
} // namespace bot