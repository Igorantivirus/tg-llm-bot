#pragma once

#include <ctime>
#include <optional>
#include <string>
#include <vector>

#include <utils/JsonSerialize.hpp>

#include "Role.hpp"

namespace dto
{
struct FunctionCall
{
    std::optional<std::string> name;
    std::string                arguments;
};
JSONSER_SIMPLE_SERIALIZATION_OUTLINE_WITH_DEFAULTS(FunctionCall);

enum class ToolCallType
{
    function
};
JSONSER_SIMPLE_SERIALIZATION_OUTLINE_WITH_DEFAULTS(ToolCallType);

struct ToolCall
{
    std::optional<ToolCallType> type;
    FunctionCall                function;
    std::optional<std::string>  id;
    std::int64_t                index{};
};
JSONSER_SIMPLE_SERIALIZATION_OUTLINE_WITH_DEFAULTS(ToolCall);

struct ResponseMessage
{
    std::optional<Role>                  role;              // "assistant"
    std::optional<std::string>           reasoning_content; // Рассуждения модели
    std::optional<std::string>           content;           // Ответ модели
    std::optional<std::vector<ToolCall>> tool_calls;        // вызовы функций модели
};
JSONSER_SIMPLE_SERIALIZATION_OUTLINE_WITH_DEFAULTS(ResponseMessage);

enum class FinishReason
{
    stop,
    length,
    tool_calls
};
JSONSER_SIMPLE_SERIALIZATION_OUTLINE_WITH_DEFAULTS(FinishReason);

struct Choice
{
    std::optional<FinishReason>    finish_reason;
    std::int64_t                   index{};
    std::optional<ResponseMessage> message; // std::nullopt, если delta
    std::optional<ResponseMessage> delta;   // std::nullopt, если message
    // std::optional<std::string>     logprobs;
};
JSONSER_SIMPLE_SERIALIZATION_OUTLINE_WITH_DEFAULTS(Choice);

struct TokenDetails
{
    std::int64_t cached_tokens{}; // Сколько токенов кешировалось
    // Неактуально для llama-cpp
    // std::int64_t reasoning_tokens{};
    // std::int64_t audio_tokens{};
    // std::int64_t accepted_prediction_tokens{};
    // std::int64_t rejected_prediction_tokens{};
};
JSONSER_SIMPLE_SERIALIZATION_OUTLINE_WITH_DEFAULTS(TokenDetails);

struct Usage
{
    std::int64_t prompt_tokens{};       // Токенов в запросе
    std::int64_t completion_tokens{};   // Токенов в ответе
    std::int64_t total_tokens{};        // Всего токенов
    TokenDetails prompt_tokens_details; // Доп информация о токенах
};
JSONSER_SIMPLE_SERIALIZATION_OUTLINE_WITH_DEFAULTS(Usage);

struct ChatCompletionsResponse
{
    std::vector<Choice>  choices;            // Ответы
    std::time_t          created{};          // Unix time
    std::string          id;                 // id
    std::string          model;              // Модель
    std::string          system_fingerprint; // отпечаток системы
    std::string          object;             // "chat.completion" | "chat.completion.chunk"
    std::optional<Usage> usage;              // Использование токенов
};
JSONSER_SIMPLE_SERIALIZATION_OUTLINE_WITH_DEFAULTS(ChatCompletionsResponse);
} // namespace dto