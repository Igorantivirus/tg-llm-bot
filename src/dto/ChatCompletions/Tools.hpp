#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>

#include "JsonSchema.hpp"

namespace dto
{

// ============================================================================
//  Объявление инструментов (уходит в запросе)
// ============================================================================

struct ToolFunction
{
    std::string                   name;
    std::optional<std::string>    description;
    std::optional<schema::Object> parameters;
    std::optional<bool>           strict;
};

struct Tool
{
    std::string  type = "function";
    ToolFunction function;
};

// ============================================================================
//  Принудительный выбор инструмента (tool_choice)
// ============================================================================

struct ChoiceFunction
{
    std::string name;
};

struct ToolChoiceObject
{
    std::string    type = "function";
    ChoiceFunction function;
};

using ToolChoice = std::variant<std::string, ToolChoiceObject>; // ("none" | "auto" | "required") либо объект.

// ============================================================================
//  Вызовы инструментов (приходят в ответе, уходят обратно в истории)
// ============================================================================

enum class ToolCallType : std::uint8_t
{
    function,
    custom
};

struct FunctionCall
{
    std::optional<std::string> name;
    std::optional<std::string> arguments; // json
};

struct ToolCall
{
    std::optional<std::int64_t> index;    // index — позиция в массиве tool_calls
    std::optional<std::string>  id;       // Приходит только в первом чанке вызова.
    std::optional<ToolCallType> type;     //
    std::optional<FunctionCall> function; // В дельте может отсутствовать целиком: чанк способен нести только id/type.
};

} // namespace dto
