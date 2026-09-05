#pragma once

#include "ChatsSettings/AdditionalsToMessage.hpp"
#include "ChatsSettings/HistoryUtils.hpp"
#include "MessagesGenerator/AssistantMessagesGenerator.hpp"

namespace openai
{

class ChatsProcessor
{
public:
    ChatsProcessor(asio::any_io_executor ex, const std::size_t tcpConnsCount, std::string host, std::string port, std::string apiToken = {})
        : api_(ex, tcpConnsCount, std::move(host), std::move(port), apiToken)
    {
    }

    const ChatsSettings &settings() const
    {
        return setts_;
    }
    ChatsSettings &settings()
    {
        return setts_;
    }

    void addTool(Tool::Ptr tool)
    {
        setts_.tools_[tool->name()] = std::move(tool);
    }

    utils::AsyncResult<const std::unordered_set<std::string> *> initModels()
    {
        auto modelsDto = co_await api_.models();
        if (!modelsDto)
            co_return std::unexpected(modelsDto.error());
        dto::ModelsResponse modelsRes = std::move(modelsDto.value());

        setts_.models_ = modelsRes.data | std::views::transform([](const dto::Model &model) -> std::string
        {
            return model.id;
        }) | std::ranges::to<std::unordered_set<std::string>>();

        co_return &setts_.models_;
    }

    utils::AsyncResult<AssistantMessagesGenerator> chatCompletions(const ChatIdType chatId, std::string msg, AdditionalsToMessage additionals = {})
    {
        dto::Message startmsg = HistoryUtils::constructStartMessage(std::move(msg), std::move(additionals));
        co_return AssistantMessagesGenerator(api_, chatId, std::move(startmsg), setts_);
    }

private:
    Api           api_;
    ChatsSettings setts_;
};

} // namespace openai