#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <utils/Jsonser.hpp>
#include <utils/NonNullCopybleUniquePtr.hpp>

namespace dto::schema
{

struct Schema;
using SchemaPtr = utils::NonNullCopybleUniquePtr<Schema>; // must be non nullptr

struct Null
{
    std::string                type = "null";
    std::optional<std::string> description;
};

struct Boolean
{
    std::string                     type = "boolean";
    std::optional<std::string>      description;
    std::optional<bool> const_field JSONSER_FIELD(const_field, "const");
};

struct String
{
    std::string                                        type = "string";
    std::optional<std::string>                         description;
    std::optional<std::vector<std::string>> enum_field JSONSER_FIELD(enum_field, "enum");
    std::optional<std::string> const_field             JSONSER_FIELD(const_field, "const");

    std::optional<std::int64_t> minLength;
    std::optional<std::int64_t> maxLength;
    std::optional<std::string>  pattern;
    std::optional<std::string>  format;
};

struct Integer
{
    std::string                                         type = "integer";
    std::optional<std::string>                          description;
    std::optional<std::vector<std::int64_t>> enum_field JSONSER_FIELD(enum_field, "enum");
    std::optional<std::int64_t> const_field             JSONSER_FIELD(const_field, "const");

    std::optional<std::int64_t> minimum;
    std::optional<std::int64_t> maximum;
    std::optional<std::int64_t> exclusiveMinimum;
    std::optional<std::int64_t> exclusiveMaximum;
    std::optional<std::int64_t> multipleOf;
};

struct Number
{
    std::string                                   type = "number";
    std::optional<std::string>                    description;
    std::optional<std::vector<double>> enum_field JSONSER_FIELD(enum_field, "enum");
    std::optional<double> const_field             JSONSER_FIELD(const_field, "const");

    std::optional<double> minimum;
    std::optional<double> maximum;
    std::optional<double> exclusiveMinimum;
    std::optional<double> exclusiveMaximum;
    std::optional<double> multipleOf;
};

struct Array
{
    std::string                type = "array";
    std::optional<std::string> description;

    std::optional<std::int64_t> minItems;
    std::optional<std::int64_t> maxItems;
    std::optional<bool>         uniqueItems;
    std::optional<SchemaPtr>    items;
};

struct Ref
{
    std::string ref            JSONSER_FIELD(ref, "$ref");
    std::optional<std::string> description;
};

struct Object
{
    std::string                type = "object";
    std::optional<std::string> description;

    std::optional<std::vector<std::string>>                   required;
    std::optional<std::variant<bool, SchemaPtr>>              additionalProperties;
    std::optional<std::int64_t>                               minProperties;
    std::optional<std::int64_t>                               maxProperties;
    std::optional<std::unordered_map<std::string, SchemaPtr>> properties;

    std::optional<std::unordered_map<std::string, SchemaPtr>> defs JSONSER_FIELD(defs, "$defs");
};

struct AnyOf
{
    std::optional<std::string> description;
    std::vector<SchemaPtr>     anyOf;
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

} // namespace dto::schema