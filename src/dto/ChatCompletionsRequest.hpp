#pragma once

#include <utils/JsonSerialize.hpp>

#include "JsonSchema.hpp"
#include "Role.hpp"

namespace dto
{
template <utils::jsonser::BasicJson J, typename... Types>
void to_json(J &j, const std::variant<Types...> &var)
{
    std::visit([&j](const auto &val)
    {
        j = val;
    }, var);
}

struct ToolFunction
{
    std::string    name;
    std::string    description;
    schema::Object properties;
};
JSONSER_SIMPLE_SERIALIZATION_OUTLINE(ToolFunction);

struct Tool
{
    std::string  type = "function";
    ToolFunction function;
    bool         strict;
};
JSONSER_SIMPLE_SERIALIZATION_OUTLINE(Tool)

struct Message
{
    Role        role;
    std::string content;
};
JSONSER_SIMPLE_SERIALIZATION_OUTLINE(Message);

struct ResponseFormat
{
    std::string                   type;
    std::optional<schema::Object> json_schema;
};
JSONSER_SIMPLE_SERIALIZATION_OUTLINE(ResponseFormat);

struct ChoiceFunction
{
    std::string name;
};
JSONSER_SIMPLE_SERIALIZATION_OUTLINE(ChoiceFunction);

struct ToolChoice
{
    std::string    type = "function";
    ChoiceFunction function;
};
JSONSER_SIMPLE_SERIALIZATION_OUTLINE(ToolChoice);

struct ChatCompletionsRequest
{
    // Основные параметры
    std::string          model;    // модель
    std::vector<Message> messages; // Сообщения

    // Необязательные числовые параметры
    std::optional<double>       temperature;       // (default = 0.8)   [0;2] Чем больше, тем больше случайного выболра ответа нейронки
    std::optional<double>       top_p;             // (default = 0.95)  Учитываются токены с кумулятивной вероятностью до top_p
    std::optional<std::int64_t> top_k;             // (default = 40)    Ограничение выборки наиболее вероятными токенами
    std::optional<double>       min_p;             // (default = 0.05)  Минимальный порог вероятности относительно самого вероятного токена
    std::optional<std::int64_t> max_tokens;        // (default = -1)    Максимальное число генерируемых токенов
    std::optional<bool>         stream;            // (default = false) Включиьт потоковую передачу
    std::optional<double>       frequency_penalty; // (default = 0.0    Штраф токенов из-за их частого появления в тексте
    std::optional<double>       presence_penalty;  // (default = 0.0)   Штраф токена за его появление в тексте
    std::optional<std::int64_t> seed;              // (default = -1)    Семsee генерации

    // Необязательные структурные поля
    std::optional<std::vector<std::string>>              stop;            // Стоп-слова
    std::optional<ResponseFormat>                        response_format; // Формат ответа
    std::optional<std::vector<Tool>>                     tools;           // Инструменты
    std::optional<std::variant<std::string, ToolChoice>> tool_choice;     // Обязательно выбора инструмента
};
JSONSER_SIMPLE_SERIALIZATION_OUTLINE(ChatCompletionsRequest);
} // namespace dto