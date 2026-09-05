#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "Role.hpp"
#include "Tools.hpp"
#include "Usage.hpp"

namespace dto
{

// ============================================================================
//  logprobs
// ============================================================================

struct TopLogprob
{
    std::string                              token;
    double                                   logprob{};
    std::optional<std::vector<std::int32_t>> bytes;
};

struct LogprobEntry
{
    std::string                              token;
    double                                   logprob{};
    std::optional<std::vector<std::int32_t>> bytes;
    std::vector<TopLogprob>                  top_logprobs;
};

struct Logprobs
{
    std::optional<std::vector<LogprobEntry>> content;
    std::optional<std::vector<LogprobEntry>> refusal;
};

// ============================================================================
//  Сообщение ответа
// ============================================================================
struct ResponseMessage
{
    std::optional<Role>                  role; // В первом чанке стрима.
    std::optional<std::string>           reasoning_content;
    std::optional<std::string>           content;
    std::optional<std::string>           refusal; // Причина отказа модели
    std::optional<std::vector<ToolCall>> tool_calls;
};

// ============================================================================
//  finish_reason
// ============================================================================

enum class FinishReason : std::uint8_t
{
    stop,           ///< Обычный ответ. Показываем пользователю, цикл закончен.
    length,         ///< Упёрлись в max_completion_tokens, ответ обрезан.
    tool_calls,     ///< Надо выполнить вызовы и сходить в API ещё раз.
    content_filter, ///< Ответ зарезан модерацией.
    function_call,  ///< Legacy, старый API functions/function_call.
};

// ============================================================================
//  Choice
// ============================================================================

struct Choice
{
    std::int64_t                   index{};
    std::optional<FinishReason>    finish_reason;   // В чанках стрима приходит null до самого последнего чанка.
    std::optional<ResponseMessage> message;         // Заполнено в нестриминговом ответе (object == "chat.completion").
    std::optional<ResponseMessage> delta;           // Заполнено в чанке стрима (object == "chat.completion.chunk").
    std::optional<Logprobs>        logprobs;        //
    std::optional<PromptProgress>  prompt_progress; // llama.cpp, при return_progress: true.
};

} // namespace dto
