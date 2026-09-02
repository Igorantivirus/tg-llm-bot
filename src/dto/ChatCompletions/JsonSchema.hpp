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

using Properties = std::unordered_map<std::string, SchemaPtr>;
using Defs = std::unordered_map<std::string, SchemaPtr>;

// ============================================================================
//  Узлы схемы
// ============================================================================

/// @brief Общие для всех узлов аннотации JSON Schema.
struct Annotations
{
    std::optional<std::string>         title;
    std::optional<std::string>         description;
    std::optional<std::string> comment JSONSER_FIELD(comment, "$comment");
};

struct Null
{
    std::string                type = "null";
    std::optional<std::string> title;
    std::optional<std::string> description;
};

struct Boolean
{
    std::string                       type = "boolean";
    std::optional<std::string>        title;
    std::optional<std::string>        description;
    std::optional<bool> const_field   JSONSER_FIELD(const_field, "const");
    std::optional<bool> default_field JSONSER_FIELD(default_field, "default");
};

struct String
{
    std::string                                        type = "string";
    std::optional<std::string>                         title;
    std::optional<std::string>                         description;
    std::optional<std::vector<std::string>> enum_field JSONSER_FIELD(enum_field, "enum");
    std::optional<std::string> const_field             JSONSER_FIELD(const_field, "const");
    std::optional<std::string> default_field           JSONSER_FIELD(default_field, "default");

    std::optional<std::int64_t> minLength;
    std::optional<std::int64_t> maxLength;
    std::optional<std::string>  pattern;
    std::optional<std::string>  format;
};

struct Integer
{
    std::string                                         type = "integer";
    std::optional<std::string>                          title;
    std::optional<std::string>                          description;
    std::optional<std::vector<std::int64_t>> enum_field JSONSER_FIELD(enum_field, "enum");
    std::optional<std::int64_t> const_field             JSONSER_FIELD(const_field, "const");
    std::optional<std::int64_t> default_field           JSONSER_FIELD(default_field, "default");

    std::optional<std::int64_t> minimum;
    std::optional<std::int64_t> maximum;
    std::optional<std::int64_t> exclusiveMinimum;
    std::optional<std::int64_t> exclusiveMaximum;
    std::optional<std::int64_t> multipleOf;
};

struct Number
{
    std::string                                   type = "number";
    std::optional<std::string>                    title;
    std::optional<std::string>                    description;
    std::optional<std::vector<double>> enum_field JSONSER_FIELD(enum_field, "enum");
    std::optional<double> const_field             JSONSER_FIELD(const_field, "const");
    std::optional<double> default_field           JSONSER_FIELD(default_field, "default");

    std::optional<double> minimum;
    std::optional<double> maximum;
    std::optional<double> exclusiveMinimum;
    std::optional<double> exclusiveMaximum;
    std::optional<double> multipleOf;
};

struct Array
{
    std::string                type = "array";
    std::optional<std::string> title;
    std::optional<std::string> description;

    std::optional<std::int64_t> minItems;
    std::optional<std::int64_t> maxItems;
    std::optional<bool>         uniqueItems;

    /// Однородный массив: items — одна схема для всех элементов.
    std::optional<SchemaPtr> items;

    /// Кортеж (JSON Schema 2020-12): позиционные схемы.
    /// В draft-07 ту же роль играл items в виде массива; llama.cpp понимает prefixItems.
    std::optional<std::vector<SchemaPtr>> prefixItems;

    std::optional<SchemaPtr> contains;
};

struct Ref
{
    std::string ref            JSONSER_FIELD(ref, "$ref");
    std::optional<std::string> description;
};

struct Object
{
    std::string                type = "object";
    std::optional<std::string> title;
    std::optional<std::string> description;

    std::optional<std::vector<std::string>>      required;
    std::optional<std::variant<bool, SchemaPtr>> additionalProperties;
    std::optional<std::int64_t>                  minProperties;
    std::optional<std::int64_t>                  maxProperties;

    std::optional<Properties> properties;
    std::optional<Properties> patternProperties;

    std::optional<Defs> defs JSONSER_FIELD(defs, "$defs");
};

struct MultiType
{
    std::vector<std::string>   type;
    std::optional<std::string> title;
    std::optional<std::string> description;
};

struct AnyOf
{
    std::optional<std::string> description;
    std::vector<SchemaPtr>     anyOf;
};

struct OneOf
{
    std::optional<std::string> description;
    std::vector<SchemaPtr>     oneOf;
};

struct AllOf
{
    std::optional<std::string> description;
    std::vector<SchemaPtr>     allOf;
};

struct Not
{
    std::optional<std::string> description;
    SchemaPtr not_field        JSONSER_FIELD(not_field, "not");
};

/// @brief Пустая схема {} — «любой JSON».
/// Совершенно легальна и прямо упомянута в документации llama.cpp
/// (-j '{}' для произвольного JSON-объекта). Без этой альтернативы
/// десериализатор спотыкался бы на валидном вводе.
struct Any
{
    std::optional<std::string> description;
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
        MultiType,
        AnyOf,
        OneOf,
        AllOf,
        Not,
        Ref,
        Any>
        value;
};

} // namespace dto::schema
