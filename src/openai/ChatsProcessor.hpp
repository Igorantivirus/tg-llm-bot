#pragma once

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <iterator>
#include <ranges>

#include <boost/asio/awaitable.hpp>

#include "Api.hpp"
#include "ChatSettings.hpp"
#include "MessageSender.hpp"
#include "SettingsRepository.hpp"
#include "Types.hpp"
#include "dto/ChatCompletionsResponse.hpp"
#include "openai/ChatSettings.hpp"
#include "utils/Types.hpp"

namespace openai
{

class ChatsProcessor
{
public:
    ChatsProcessor(asio::any_io_executor ex, std::string host, std::string port, std::string token = {})
        : ex_(ex),
          api_(ex, std::move(host), std::move(port), std::move(token))
    {
    }

    void setSender(MessageSender &sender)
    {
        sender_ = &sender;
    }

    const std::unordered_set<std::string> &getModels() const
    {
        return repo_.getModels();
    }
    const ChatSettings &getSettings(const ChatID chatId)
    {
        return repo_.getSettings(chatId);
    }
    void setModelToChat(const ChatID chatId, std::string model)
    {
        repo_.setModelToChat(chatId, std::move(model));
    }

    void initModels()
    {
        asio::co_spawn(ex_, initModelsSync(), asio::detached);
    }

    void sendMessage(const ChatID chatId, std::string msg, const dto::Role role = dto::Role::user)
    {
        asio::co_spawn(ex_, sendMessageSync(chatId, std::move(msg), role), asio::detached);
    }

    utils::AsyncResult<void> initModelsSync()
    {
        auto modelsDto = co_await api_.models();
        if (!modelsDto)
            co_return std::unexpected(modelsDto.error());
        dto::ModelsResponse modelsRes = std::move(modelsDto.value());

        std::unordered_set<std::string> &&models = modelsRes.data | std::views::transform([](const dto::Model &model) -> std::string
        {
            return model.id;
        }) | std::ranges::to<std::unordered_set<std::string>>();

        repo_.setModels(models);
        co_return utils::empty;
    }

    utils::AsyncResult<void> sendMessageSync(const ChatID chatId, std::string msg, const dto::Role role = dto::Role::user)
    {
        repo_.addMessage(chatId, msg, role);

        ChatSettings setts = repo_.getSettings(chatId);
        if (setts.model.empty())
        {
            sender_->sendError(chatId, Error::NoSetModel);
            co_return utils::empty;
        }
        dto::ChatCompletionsRequest req;
        initChatCompletionsRequestBySettings(req, setts);

        auto response = co_await api_.chatCompletions(std::move(req));
        if (!response)
        {
            sender_->sendError(chatId, response.error());
            co_return utils::empty;
        }
        auto var = std::move(response.value());
        if (var.index() == 0) // не будет покка styream = true
            co_return sendMessage(std::get<0>(var), chatId);
        else
            co_return co_await streamMessages(std::get<1>(var), chatId);
    }

private:
    asio::any_io_executor ex_;
    Api                   api_;
    SettingsRepository    repo_;
    MessageSender        *sender_;

private:
    utils::SyncResult<void> sendMessage(dto::ChatCompletionsResponse &dto, const ChatID chatId)
    {
        if (dto.choices.empty())
            return std::unexpected(Error::EmptyResponse);

        dto::Choice &ch = dto.choices[0];
        if (!ch.message || !ch.message->role || !ch.message->content) // пустое сообщение // TODO: Когда будут tools - сделать обработку вызовов
            return std::unexpected(Error::EmptyResponse);

        repo_.addMessage(chatId, *ch.message->content, *ch.message->role);
        sender_->sendMessage(chatId, std::move(*ch.message->content));

        return utils::empty;
    }
    void nextMessageToUser(std::string msg, const ChatID chatId, std::optional<MessageID> &msgIdOpt)
    {
        if (!msgIdOpt)
            msgIdOpt = sender_->sendMessage(chatId, std::move(msg));
        else
            sender_->replaceMessage(chatId, *msgIdOpt, std::move(msg));
    }
    utils::AsyncResult<void> streamMessages(ApiResponseGenerator &gen, const ChatID chatId)
    {
        std::string              messageAccum;
        std::size_t              lastSizeAccum = 0;
        const std::size_t        criticalMass = 30;
        auto                     next = co_await gen.next();
        std::optional<MessageID> msgIdOpt = std::nullopt;
        while (next && next.value())
        {
            auto dto = next.value().value();

            messageAccum += extractContentFromDelta(dto);
            if (messageAccum.size() >= lastSizeAccum + criticalMass) // добавилось больше, чем критическая масса
            {
                nextMessageToUser(messageAccum, chatId, msgIdOpt);
                lastSizeAccum = messageAccum.size();
            }

            next = next = co_await gen.next();
        }
        if (messageAccum.size() > lastSizeAccum)
            nextMessageToUser(std::move(messageAccum), chatId, msgIdOpt);
        if (!next)
            co_return std::unexpected(next.error());

        co_return utils::empty;
    }

private:
    static std::string extractContentFromDelta(const dto::ChatCompletionsResponse &dto)
    {
        if (dto.choices.empty())
            return "";

        const dto::Choice &ch = dto.choices[0];
        if (!ch.delta || !ch.delta->role || !ch.delta->content) // пустое сообщение // TODO: Когда будут tools - сделать обработку вызовов
            return "";
        return *ch.delta->content;
    }

    static void initChatCompletionsRequestBySettings(dto::ChatCompletionsRequest &req, ChatSettings &setts)
    {
        auto &&pr = setts.mesages | std::views::transform([](std::pair<dto::Role, std::string> &msg) -> dto::Message
        {
            return dto::Message{.role = msg.first, .content = std::move(msg.second)};
        }) | std::ranges::to<decltype(dto::ChatCompletionsRequest::messages)>();

        if (!setts.systemPromt.empty())
            req.messages.push_back(dto::Message{.role = dto::Role::system, .content = std::move(setts.systemPromt)});
        req.messages.insert(req.messages.end(), std::make_move_iterator(pr.begin()), std::make_move_iterator(pr.end()));
        req.model = std::move(setts.model);
        req.stream = false;
    }
};

} // namespace openai