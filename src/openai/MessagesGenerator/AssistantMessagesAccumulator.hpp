#pragma once

#include <algorithm>
#include <ranges>
#include <stdexcept>

#include <dto/ChatCompletions/Message.hpp>
#include <dto/ChatCompletions/Response.hpp>
#include <dto/ChatCompletions/Tools.hpp>
#include <utils/MethodBinder.hpp>

#include <openai/ChatsSettings/ChatsSettings.hpp>

namespace openai
{
class AssistantMessagesAccumulator
{
public:
    void addMessage(dto::Message startMsg)
    {
        accumFragment_.push_back(startMsg);
    }
    void addMessages(std::vector<dto::Message> msg)
    {
        accumFragment_.insert(accumFragment_.end(), std::make_move_iterator(msg.begin()), std::make_move_iterator(msg.end()));
    }

    void accumulate(dto::ChatCompletionsResponse resp)
    {
        if (resp.choices.size() != 1)
            return;
        dto::Choice           choice = std::move(resp.choices[0]);
        dto::ResponseMessage *msg = choice.message ? &(choice.message.value()) : (choice.delta ? &choice.delta.value() : nullptr);
        if (!msg)
            return;

        reason_ = std::nullopt;

        if (msg->content)
            content_ += std::move(msg->content.value());
        if (msg->reasoning_content)
            reasoningContent_ += std::move(msg->reasoning_content.value());
        if (msg->refusal)
            refusal_ += std::move(msg->refusal.value());
        if (msg->tool_calls)
            std::for_each_n(msg->tool_calls->begin(), msg->tool_calls->size(), utils::buildMethod(&AssistantMessagesAccumulator::accumulateTool, this));

        if (choice.finish_reason)
            finish(choice.finish_reason.value());
    }

    bool isFinish()
    {
        return reason_.has_value();
    }
    dto::FinishReason getReason() const
    {
        if (reason_)
            return reason_.value();
        throw std::logic_error("Reason not inited yet.");
    }
    dto::Message getLastMessage()
    {
        if (reason_) // Есди есть причина окончания, значит и точно есть последнее сообщение
            return accumFragment_.back();
        throw std::logic_error("Reason not inited yet.");
    }

    const DialogFragment &getDialogFragment() const
    {
        return accumFragment_;
    }
    DialogFragment pullDialogFragment()
    {
        return std::move(accumFragment_);
    }

private:
    DialogFragment accumFragment_; // Аккумулирование всей истории

    std::optional<dto::FinishReason> reason_; // Финальная причина конца

    std::string                                     content_;          // Аккумулирование ответа
    std::string                                     reasoningContent_; // Аккумулирование рассуждений
    std::string                                     refusal_;          // Аккумулирование отказа
    std::unordered_map<std::int64_t, dto::ToolCall> tools_;            // Аккумулирование вызова функий {index: tool}

    std::int64_t notIndexedIndexAccumulator_ = 0; // Если у нас не потоковое чтение, то индексы делаем сами

private:
    void clear()
    {
        content_.clear();
        reasoningContent_.clear();
        refusal_.clear();
        tools_.clear();
    }

    void finish(const dto::FinishReason reason)
    {
        dto::Message msg;
        msg.role = dto::Role::assistant;
        msg.content = std::move(content_);
        msg.refusal = std::move(refusal_);

        msg.tool_calls = tools_ | std::views::values | std::views::filter([](const dto::ToolCall &tc) -> bool
        {
            return tc.id && tc.type && tc.function && tc.function->name && tc.function->arguments;
        }) | std::views::as_rvalue |
                         std::ranges::to<std::vector<dto::ToolCall>>();

        clear();
        reason_ = reason;
        accumFragment_.push_back(std::move(msg));
    }

    void accumulateTool(dto::ToolCall &tool)
    {
        std::int64_t index = tool.index ? tool.index.value() : --notIndexedIndexAccumulator_;

        static auto checkAndSet = []<typename T>(std::optional<T> &to, std::optional<T> &from)
        {
            if (from && !to)
                to = std::move(from);
        };
        static auto emplaceIfNo = []<typename T>(std::optional<T> &v)
        {
            if (!v)
                v.emplace();
        };

        dto::ToolCall &toolCall = tools_[index];

        checkAndSet(toolCall.id, tool.id);
        checkAndSet(toolCall.type, tool.type);

        if (tool.function)
        {
            emplaceIfNo(toolCall.function);
            checkAndSet(toolCall.function->name, tool.function->name);
            emplaceIfNo(toolCall.function->arguments);
            toolCall.function->arguments.value() += tool.function->arguments.value_or({});
        }
    }
};
} // namespace openai