#pragma once

#include <dto/Utils.hpp>

#include <openai/Api/Api.hpp>
#include <openai/Api/ApiResponseGenerator.hpp>
#include <openai/ChatsSettings/ChatsSettings.hpp>
#include <openai/ChatsSettings/HistoryUtils.hpp>

#include "AssistantMessagesAccumulator.hpp"
#include "AssistantToolCaller.hpp"

namespace openai
{

class AssistantMessagesGenerator : public utils::StreamGenerator<std::string>
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
    utils::AsyncResult<std::string> nextImpl() override
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

            accumulator_.accumulate(std::move(resp));

            if (accumulator_.isFinish()) // если конец - его надо обработать
            {
                if (auto finistRes = co_await processFinish(); !finistRes)
                    co_return std::unexpected(finistRes.error());
            }
            if (content)
                co_return content.value();
        }

        co_return std::unexpected(apiGen_->endReason());
    }

    utils::AsyncResult<void> processFinish()
    {
        if (dto::FinishReason reason = accumulator_.getReason(); reason == dto::FinishReason::tool_calls)
        {
            accumulator_.addMessages(co_await tcler_.callsTools(accumulator_.getLastMessage()));
            if (auto initRes = co_await initApiGen(); !initRes) // Продолжаем генерацию
                co_return std::unexpected(initRes.error());
        }
        co_return utils::empty;
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