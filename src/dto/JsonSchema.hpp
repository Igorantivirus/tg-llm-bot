#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <utils/JsonSerialize.hpp>
#include <utils/NonNullCopybleUniquePtr.hpp>

namespace dto::schema
{

struct Schema;
using SchemaPtr = utils::NonNullCopybleUniquePtr<Schema>; // must be non nullptr

struct Null
{
    std::string                JSONSER_FDEFN(type = "null");
    std::optional<std::string> JSONSER_FDEFN(description);
};

struct Boolean
{
    std::string                JSONSER_FDEFN(type = "boolean");
    std::optional<std::string> JSONSER_FDEFN(description);
    std::optional<bool>        JSONSER_FIELD(const_field, "const");
};

struct String
{
    std::string                             JSONSER_FDEFN(type = "string");
    std::optional<std::string>              JSONSER_FDEFN(description);
    std::optional<std::vector<std::string>> JSONSER_FIELD(enum_field, "enum");
    std::optional<std::string>              JSONSER_FIELD(const_field, "const");

    std::optional<std::int64_t> JSONSER_FDEFN(minLength);
    std::optional<std::int64_t> JSONSER_FDEFN(maxLength);
    std::optional<std::string>  JSONSER_FDEFN(pattern);
    std::optional<std::string>  JSONSER_FDEFN(format);
};

struct Integer
{
    std::string                              JSONSER_FDEFN(type = "integer");
    std::optional<std::string>               JSONSER_FDEFN(description);
    std::optional<std::vector<std::int64_t>> JSONSER_FIELD(enum_field, "enum");
    std::optional<std::int64_t>              JSONSER_FIELD(const_field, "const");

    std::optional<std::int64_t> JSONSER_FDEFN(minimum);
    std::optional<std::int64_t> JSONSER_FDEFN(maximum);
    std::optional<std::int64_t> JSONSER_FDEFN(exclusiveMinimum);
    std::optional<std::int64_t> JSONSER_FDEFN(exclusiveMaximum);
    std::optional<std::int64_t> JSONSER_FDEFN(multipleOf);
};

struct Number
{
    std::string                        JSONSER_FDEFN(type = "number");
    std::optional<std::string>         JSONSER_FDEFN(description);
    std::optional<std::vector<double>> JSONSER_FIELD(enum_field, "enum");
    std::optional<double>              JSONSER_FIELD(const_field, "const");

    std::optional<double> JSONSER_FDEFN(minimum);
    std::optional<double> JSONSER_FDEFN(maximum);
    std::optional<double> JSONSER_FDEFN(exclusiveMinimum);
    std::optional<double> JSONSER_FDEFN(exclusiveMaximum);
    std::optional<double> JSONSER_FDEFN(multipleOf);
};

struct Array
{
    std::string                JSONSER_FDEFN(type = "array");
    std::optional<std::string> JSONSER_FDEFN(description);

    std::optional<std::int64_t> JSONSER_FDEFN(minItems);
    std::optional<std::int64_t> JSONSER_FDEFN(maxItems);
    std::optional<bool>         JSONSER_FDEFN(uniqueItems);
    std::optional<SchemaPtr>    JSONSER_FDEFN(items);
};

struct Ref
{
    std::string                JSONSER_FIELD(ref, "$ref");
    std::optional<std::string> JSONSER_FDEFN(description);
};

struct Object
{
    std::string                JSONSER_FDEFN(type = "object");
    std::optional<std::string> JSONSER_FDEFN(description);

    std::optional<std::vector<std::string>>                   JSONSER_FDEFN(required);
    std::optional<std::variant<bool, SchemaPtr>>              JSONSER_FDEFN(additionalProperties);
    std::optional<std::int64_t>                               JSONSER_FDEFN(minProperties);
    std::optional<std::int64_t>                               JSONSER_FDEFN(maxProperties);
    std::optional<std::unordered_map<std::string, SchemaPtr>> JSONSER_FDEFN(properties);

    std::optional<std::unordered_map<std::string, SchemaPtr>> JSONSER_FIELD(defs, "$defs");
};

struct AnyOf
{
    std::optional<std::string> JSONSER_FDEFN(description);
    std::vector<SchemaPtr>     JSONSER_FDEFN(anyOf);
};

struct Schema
{
    std::variant<
        Null,
        Boolean,
        String,
        Integer,
        Number,
        Array,
        Object,
        AnyOf,
        Ref>
        value;
};

template <utils::jsonser::BasicJson J, typename... Types>
void to_json(J &j, const std::variant<Types...> &var)
{
    std::visit([&j](const auto &val)
    {
        j = val;
    }, var);
}

template <utils::jsonser::BasicJson J>
void to_json(J &j, const SchemaPtr &ptr)
{
    j = ptr->value;
}
template <utils::jsonser::BasicJson J>
void from_json(const J &j, SchemaPtr &ptr)
{
    throw std::logic_error("It is forbidden to call deserialization structures with JsonSchema.");
}

JSONSER_SERIALIZATION_OUTLINE_WITH_DEFAULTS(Null);
JSONSER_SERIALIZATION_OUTLINE_WITH_DEFAULTS(Boolean);
JSONSER_SERIALIZATION_OUTLINE_WITH_DEFAULTS(String);
JSONSER_SERIALIZATION_OUTLINE_WITH_DEFAULTS(Integer);
JSONSER_SERIALIZATION_OUTLINE_WITH_DEFAULTS(Number);
JSONSER_SERIALIZATION_OUTLINE_WITH_DEFAULTS(Array);
JSONSER_SERIALIZATION_OUTLINE_WITH_DEFAULTS(Ref);
JSONSER_SERIALIZATION_OUTLINE_WITH_DEFAULTS(Object);
JSONSER_SERIALIZATION_OUTLINE_WITH_DEFAULTS(AnyOf);

} // namespace dto::schema