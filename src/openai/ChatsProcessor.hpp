#pragma once

#include <iterator>
#include <ranges>

#include <boost/asio/awaitable.hpp>

#include "Api.hpp"
#include "ChatSettings.hpp"
#include "MessageSender.hpp"
#include "SettingsRepository.hpp"
#include "Types.hpp"
#include "dto/ChatCompletionsResponse.hpp"

namespace openai
{

class ChatsProcessor
{
public:
    ChatsProcessor(asio::any_io_executor ex, MessageSender &sender, std::string host, std::string port, std::string token = {})
        : api_(ex, std::move(host), std::move(port), std::move(token)),
          sender_(sender)
    {
    }

    const std::unordered_set<std::string> &getModels() const
    {
        return repo_.getModels();
    }
    void setModel(const ChatID chatId, std::string model)
    {
        repo_.setModelToChat(chatId, std::move(model));
    }

    asio::awaitable<utils::ErrorCode> initModels()
    {
        auto modelsDto = co_await api_.models();
        if (!modelsDto)
            co_return modelsDto.error();
        dto::ModelsResponse modelsRes = std::move(modelsDto.value());

        std::unordered_set<std::string> &&models = modelsRes.data | std::views::transform([](const dto::Model &model) -> std::string
        {
            return model.id;
        }) | std::ranges::to<std::unordered_set<std::string>>();

        repo_.setModels(models);
        co_return Error::Success;
    }

    asio::awaitable<utils::ErrorCode> sendMessage(const ChatID chatId, std::string msg)
    {
        ChatSettings setts = repo_.getSettings(chatId);
        if (setts.model.empty())
        {
            sender_.sendError(chatId, Error::NoSetModel);
            co_return Error::Success;
        }
        auto &&pr = setts.mesages | std::views::transform([](std::pair<dto::Role, std::string> &msg) -> dto::Message
        {
            return dto::Message{.role = msg.first, .content = std::move(msg.second)};
        }) | std::ranges::to<decltype(dto::ChatCompletionsRequest::messages)>();

        dto::ChatCompletionsRequest req;
        req.model = std::move(setts.model);
        if (!setts.systemPromt.empty())
            req.messages.push_back(dto::Message{.role = dto::Role::system, .content = std::move(setts.systemPromt)});
        req.messages.insert(req.messages.end(), std::make_move_iterator(pr.begin()), std::make_move_iterator(pr.end()));
        req.messages.push_back(dto::Message{.role = dto::Role::user, .content = std::move(msg)});
        req.stream = false;

        auto response = co_await api_.chatCompletions(std::move(req));
        if (!response)
        {
            sender_.sendError(chatId, response.error());
            co_return Error::Success;
        }
        auto var = std::move(response.value());
        if (var.index() == 0) // не будет покка styream = true
            co_return sendOneMsg(std::get<0>(var), chatId);
        else
            co_return streamMessages(std::get<1>(var));
    }

private:
    Api                api_;
    SettingsRepository repo_;
    MessageSender     &sender_;

private:
    utils::ErrorCode sendOneMsg(dto::ChatCompletionsResponse &dto, const ChatID chatId)
    {
        if (dto.choices.empty())
            return Error::EmptyResponse;
        dto::Choice &ch = dto.choices[0];
        if (!ch.message)
            return Error::EmptyResponse;
        if (!ch.message->role || !ch.message->content)
            return Error::EmptyResponse;

        repo_.addMessage(chatId, *ch.message->content, *ch.message->role);

        sender_.sendMessage(chatId, std::move(*ch.message->content));

        return Error::Success;
    }
    utils::ErrorCode streamMessages(openai::ApiResponseGenerator &gen)
    {

        return {};
    }
};

} // namespace openai