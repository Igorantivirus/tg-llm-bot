#pragma once

#include "AssistentMessage.hpp"
#include "openai/dto/ChatCompletions/Message.hpp"
#include <iterator>
#include <openai/Api/Api.hpp>
#include <openai/ChatsSettings/ChatsSettings.hpp>
#include <openai/ChatsSettings/HistoryUtils.hpp>
#include <openai/dto/Utils.hpp>
#include <openai/messagegenerators/AssistantMessagesAccumulator.hpp>
#include <openai/messagegenerators/AssistantToolCaller.hpp>

namespace openai
{

class AssistantMessagesGenerator : public utils::StreamGenerator<AssistentMessage>
{
public:
    AssistantMessagesGenerator(Api &api, const ChatIdType chatId, dto::Message msg, ChatsSettings &setts)
        : api_(api),
          chatId_(chatId),
          tcler_(setts.tools()),
          setts_(setts)
    {
        accumulator_.addMessage(std::move(msg));
    }

    AssistantMessagesGenerator(AssistantMessagesGenerator &&) = default;
    AssistantMessagesGenerator(const AssistantMessagesGenerator &) = delete;
    AssistantMessagesGenerator &operator=(const AssistantMessagesGenerator &) = delete;
    AssistantMessagesGenerator &operator=(AssistantMessagesGenerator &&) = delete;

private:
    AssistantMessagesAccumulator accumulator_; // Аккумулятор

    Api                &api_;    // api доступа
    ChatIdType          chatId_; // id чата
    AssistantToolCaller tcler_;  // Вызыватель инструментов
    ChatsSettings      &setts_;  // Все данные переписок

    std::optional<ApiResponseGenerator> apiGen_; // Генератор dto'шек

private:
    // перед закрытием надо сохранить аккумулятор
    void close() override
    {
        setts_.repo().addDialogFragment(chatId_, accumulator_.pullDialogFragment()); // перед концом надо сохранить аккумулированное сообщение
    }

    // Возаращает строку ответ
    utils::AsyncResult<AssistentMessage> nextImpl() override
    {
        if (!apiGen_)
        {
            if (auto initRes = co_await initApiGen(); !initRes)
                co_return std::unexpected(initRes.error());
        }

        while (auto next = co_await apiGen_->next())
        {
            dto::ChatCompletionsResponse resp = std::move(next.value());
            std::optional<std::string>   content = dto::Utils::findContent(resp);

            AssistentMessage response = accumulator_.accumulate(std::move(resp));

            if (accumulator_.isFinish()) // если конец - его надо обработать
            {
                if (auto finistRes = co_await processFinish(response); !finistRes)
                    co_return std::unexpected(finistRes.error());
            }
            if (!response.empty())
                co_return response;
        }

        co_return std::unexpected(apiGen_->endReason());
    }

    utils::AsyncResult<void> processFinish(AssistentMessage &message)
    {
        if (dto::FinishReason reason = accumulator_.getReason(); reason == dto::FinishReason::tool_calls)
        {
            auto [frags, results] = co_await tcler_.callsTools(accumulator_.getLastMessage());

            accumulator_.addMessages(std::move(frags));
            addAdditionalMessagesAfterTool(results);

            message.toolCallResult.insert(message.toolCallResult.end(), std::make_move_iterator(results.begin()), std::make_move_iterator(results.end()));
            if (auto initRes = co_await initApiGen(); !initRes) // Продолжаем генерацию
                co_return std::unexpected(initRes.error());
        }
        co_return utils::empty;
    }

    void addAdditionalMessagesAfterTool(std::vector<ToolResult::Ptr> &results)
    {
        std::vector<dto::ContentPart> content;
        dto::TextPart                 tp;
        tp.text = "The results of the functions are attached by the system:";
        content.push_back(std::move(tp));

        for (auto &result : results)
        {
            if (!result->needToSendAdditionalMessage())
                continue;
            auto parts = result->getAdditionalMessage();
            content.insert(content.end(), std::make_move_iterator(parts.begin()), std::make_move_iterator(parts.end()));
        }
        if (content.size() == 1)
            return;
        
        dto::Message postfixMsg;
        postfixMsg.role = dto::Role::user;
        postfixMsg.content = std::move(content);
        accumulator_.addMessage(std::move(postfixMsg));
    }

    utils::AsyncResult<void> initApiGen()
    {
        auto res = co_await api_.chatCompletions(HistoryUtils::makeRequest(setts_, setts_.repo().getHistoryById(chatId_), accumulator_.getDialogFragment()));
        if (!res)
            co_return std::unexpected(res.error());
        apiGen_.emplace(std::move(res.value()));
        co_return utils::empty;
    }
};

} // namespace openai