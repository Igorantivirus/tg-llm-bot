#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "JsonSchema.hpp"

namespace dto
{

enum class ResponseFormatType : std::uint8_t
{
    text,
    json_object,
    json_schema
};

struct JsonSchemaWrapper
{
    std::string                name;
    std::optional<std::string> description;
    std::optional<bool>        strict;
    schema::Schema             schema;
};

struct ResponseFormat
{
    ResponseFormatType               type = ResponseFormatType::text;
    std::optional<JsonSchemaWrapper> json_schema;
};

} // namespace dto
