#pragma once

#include <cstdio>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

#include <boost/pfr/core.hpp>
#include <boost/pfr/core_name.hpp>
#include <boost/pfr/tuple_size.hpp>
#include <magic_enum/magic_enum.hpp>

namespace jsonser
{

template <std::size_t N>
struct StringLiteral
{
    constexpr StringLiteral() = default;
    constexpr StringLiteral(const char (&str)[N])
    {
        std::copy_n(str, N, string);
    }
    constexpr StringLiteral(const std::string_view sv)
    {
        for (std::size_t i = 0; i < N && i < sv.size(); ++i)
            string[i] = sv[i];
    }
    [[nodiscard]] constexpr std::size_t size() const
    {
        return N - 1;
    }
    [[nodiscard]] constexpr bool operator==(const StringLiteral sl) const
    {
        for (std::size_t ind = 0; ind < N; ++ind)
            if (string[ind] != sl.string[ind])
                return false;
        return true;
    }
    char string[N]{};
};
template <std::size_t N>
StringLiteral(const char (&)[N]) -> StringLiteral<N>;

template <StringLiteral S>
struct Tag
{
};

using FieldInfo = std::optional<std::string_view>;

template <typename T, StringLiteral S>
concept HasTag = requires {
    { T::jsonserMetaMethod(Tag<S>{}) } -> std::same_as<FieldInfo>;
};

template <typename J>
concept BasicJson = requires {
    typename std::remove_cvref_t<J>::object_t;
    typename std::remove_cvref_t<J>::string_t;
    typename std::remove_cvref_t<J>::exception;
};

template <typename T>
concept AgregatStructure = requires {
    requires std::is_aggregate_v<T>;
    requires !std::is_polymorphic_v<T>;
    requires std::is_default_constructible_v<T>;
    requires std::is_move_assignable_v<T>;
    requires !std::is_union_v<T>;
};

template <typename T>
struct is_variant : std::false_type
{
};

template <typename... Types>
struct is_variant<std::variant<Types...>> : std::true_type
{
};

template <typename T>
constexpr bool is_variant_v = is_variant<T>::value;

template <typename T>
concept Variant = requires {
    requires is_variant_v<T>;
};

template <typename T>
struct is_optional : std::false_type
{
};

template <typename T>
struct is_optional<std::optional<T>> : std::true_type
{
};

template <typename T>
constexpr bool is_optional_v = is_optional<T>::value;

template <typename T>
concept Optional = requires {
    requires is_optional_v<T>;
};

template <typename T>
concept NullOpt = std::is_same_v<T, std::nullopt_t>;

template <typename T>
concept Enum = requires {
    requires std::is_enum_v<T>;
};

template <typename J, typename T>
concept Serializeable = std::is_constructible_v<J, T>;

template <typename J, typename T>
concept Deserializeable = requires(const J &j, T &t) {
    { j.get_to(t) };
};

template <typename Type, typename... Types>
concept ContainsType = (std::is_same_v<Type, Types> || ...);

enum class OnMissingValue : std::uint8_t
{
    Exception,
    DefaultFromStruct,
    DefaultFromType
};
enum class OnMissingOptional : std::uint8_t
{
    Exception,
    DefaultFromStruct,
    Nullopt
};
template <OnMissingValue DefaultAction = OnMissingValue::DefaultFromStruct, OnMissingOptional DefaultOption = OnMissingOptional::Nullopt>
class Deserialize
{
public:
    template <typename... Types, BasicJson J, typename T>
    constexpr static void fromJson(const J &j, T &v)
    {
        if constexpr (ContainsType<T, Types...>) // Если этот тип запретили дусериализовывать - сразу пропускаем
            return;
        static_assert(Deserializeable<J, T> || Enum<T> || AgregatStructure<T> || Optional<T>, "Type must be enum, agregate struct or must have from_json function.");
        if constexpr (Enum<T>)
            return enumFromJson(j, v);
        else if constexpr (Optional<T>)
            return optionalFromJson(j, v);
        else if constexpr (Deserializeable<J, T>)
            return j.get_to(v), void();
        else if constexpr (AgregatStructure<T>)
            return structFromJson(j, v);
    }

private:
    template <BasicJson J, Enum E>
    static constexpr void enumFromJson(const J &j, E &e)
    {
        // Надо строго сконвертировать уже существующее поле j, поэтому with defaults тут не нужен, надо либо сконвертировать, либо ошибка
        if (!j.is_string())
            throw std::logic_error("Enum in json must be string.");
        std::string_view sv = j.template get<std::string_view>();
        auto             value = magic_enum::enum_cast<E>(sv);
        if (!value.has_value())
            throw std::logic_error("Invalid enum value " + std::string(sv) + '.');
        e = value.value();
    }

    template <BasicJson J, Optional T>
    static constexpr void optionalFromJson(const J &j, T &v)
    {
        // json поле уже есть, default не нужны
        if (j.is_null())
            return (v = std::nullopt), void();

        using InternalType = typename T::value_type;
        InternalType internal{};
        fromJson<J, InternalType>(j, internal);
        v = std::move(internal);
    }

    template <BasicJson J, typename T>
    static constexpr void fromJsonName(const J &j, const std::string_view name, T &v, T &&defValue)
    {
        if (!j.contains(name))
        {
            if constexpr (Optional<T>)
            {
                if constexpr (DefaultOption == OnMissingOptional::DefaultFromStruct)
                    v = std::forward<T>(defValue);
                else if constexpr (DefaultOption == OnMissingOptional::Nullopt)
                    v = std::nullopt;
                else
                    throw std::logic_error("Json not contains optional " + std::string(name) + " field.");
            }
            else
            {
                if constexpr (DefaultAction == OnMissingValue::DefaultFromStruct)
                    v = std::forward<T>(defValue);
                else if constexpr (DefaultAction == OnMissingValue::DefaultFromType)
                    v = T{};
                else
                    throw std::logic_error("Json not contains " + std::string(name) + " field.");
            }
        }
        else
            fromJson(j.at(name), v);
    }

    template <BasicJson J, AgregatStructure S>
    static constexpr void structFromJson(const J &j, S &v)
    {
        // Уже дали конкретное поле json - j и куда его надо десериализовать, withDefault тут не нужен, надо либо десериализовать, либо ошибка
        if (!j.is_object())
            throw std::logic_error("To convert json to struct, json must be is_object()");
        S    defStruct{};
        auto convertOne = [&j, &v, &defStruct]<std::size_t I>(std::integral_constant<std::size_t, I>)
        {
            using FieldType = boost::pfr::tuple_element_t<I, S>;
            constexpr std::string_view               name = boost::pfr::get_name<I, S>();
            constexpr StringLiteral<name.size() + 1> literal(name);
            if constexpr (HasTag<S, literal>)
            {
                constexpr FieldInfo info = S::jsonserMetaMethod(Tag<literal>{});
                if constexpr (info.has_value())
                    fromJsonName<J, FieldType>(j, *info, boost::pfr::get<I>(v), std::move(boost::pfr::get<I>(defStruct)));
            }
            else
                fromJsonName<J, FieldType>(j, name, boost::pfr::get<I>(v), std::move(boost::pfr::get<I>(defStruct)));
        };
        auto convertFoo = [&convertOne]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            (convertOne(std::integral_constant<std::size_t, Is>{}), ...);
        };
        convertFoo(std::make_index_sequence<boost::pfr::tuple_size_v<S>>{});
    }
};

class Serialize
{
public:
    template <typename... Types, BasicJson J, typename T>
    static constexpr void toJson(J &j, const T &v)
    {
        j.clear();
        if constexpr (ContainsType<T, Types...>) // Если этот тип запретили сериализовывать - сразу пропускаем
            return;
        static_assert(Serializeable<J, T> || Enum<T> || AgregatStructure<T> || Variant<T> || NullOpt<T>, "Type must be enum, agregate struct or must have to_json function.");
        if constexpr (Enum<T>)
            return enumToJson(j, v);
        else if constexpr (Variant<T>)
            return variantToJson(j, v);
        else if constexpr (Serializeable<J, T>)
            return (j = v), void();
        else if constexpr (AgregatStructure<T>)
            return structToJson<Types..., J, T>(j, v);
    }

private:
    template <BasicJson J, Variant V>
    static constexpr void variantToJson(J &j, const V &v)
    {
        // Что-то вызовется гаарантировано
        std::visit([&j]<typename T>(const T &v)
        {
            toJson(j, v);
        }, v);
    }
    template <BasicJson J, Enum E>
    static constexpr void enumToJson(J &j, const E &e)
    {
        auto name = magic_enum::enum_name(e);
        if (name.empty())
            throw std::logic_error("Enum value is out of magic_enum support range.");
        j = name;
    }

    template <typename... Types, BasicJson J, typename T>
    static constexpr void toJsonName(J &j, const std::string_view name, const T &v)
    {
        if constexpr (Optional<T>)
        {
            if (v.has_value()) // если есть значение - сразу делаем, неважно, надо или не надо сериализовывать
                toJson<Types..., J, T>(j[name], *v);
            else if (!v.has_value() && !ContainsType<std::nullopt_t, Types...>) // Если нет значения, но nullopt надо сериализовывать (нет в списке исключений) - делаем прямой вызов, там nlohmann сам сделает null
                toJson<Types..., J, T>(j[name], std::nullopt);
        }
        else // Тип допустим, optional обработали - можно напрямую сериализовывать
            toJson(j[name], v);
    }

    template <typename... Types, BasicJson J, AgregatStructure S>
    static constexpr void structToJson(J &j, const S &v)
    {
        j = J::object();

        auto convertOne = [&j, &v]<std::size_t I>(std::integral_constant<std::size_t, I>)
        {
            using FieldType = boost::pfr::tuple_element_t<I, S>;
            constexpr std::string_view               name = boost::pfr::get_name<I, S>();
            constexpr StringLiteral<name.size() + 1> literal(name);
            if constexpr (HasTag<S, literal>)
            {
                constexpr FieldInfo info = S::jsonserMetaMethod(Tag<literal>{});
                if constexpr (info.has_value())
                    toJsonName<Types..., J, S>(j, info.value(), boost::pfr::get<I>(v));
            }
            else
                toJsonName<Types..., J, S>(j, name, boost::pfr::get<I>(v));
        };
        auto convertFoo = [&convertOne]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            (convertOne(std::integral_constant<std::size_t, Is>{}), ...);
        };
        convertFoo(std::make_index_sequence<boost::pfr::tuple_size_v<S>>{});
    }
};

} // namespace jsonser

#ifndef JSONSER_FIELD
#define JSONSER_FIELD(FIELD, NAME)                                                     \
    ;                                                                                  \
    consteval inline static jsonser::FieldInfo jsonserMetaMethod(jsonser::Tag<#FIELD>) \
    {                                                                                  \
        return NAME;                                                                   \
    }
#endif

#ifndef JSONSER_SKIP
#define JSONSER_SKIP(FIELD)                                                            \
    ;                                                                                  \
    consteval inline static jsonser::FieldInfo jsonserMetaMethod(jsonser::Tag<#FIELD>) \
    {                                                                                  \
        return std::nullopt;                                                           \
    }
#endif