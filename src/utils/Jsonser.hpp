#pragma once

#include <array>
#include <cstdio>
#include <nlohmann/adl_serializer.hpp>
#include <stdexcept>
#include <string>
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

template <typename T>
concept AgregatStructure = requires {
    requires std::is_aggregate_v<T>;
    requires !std::is_polymorphic_v<T>;
    requires std::is_default_constructible_v<T>;
    requires std::is_move_assignable_v<T>;
    requires !std::is_union_v<T>;
};

template <typename T>
concept ReflectStruct = AgregatStructure<T> && !std::ranges::range<T> && !Optional<T> && !Variant<T> && !Enum<T>;

template <AgregatStructure S>
consteval std::array<FieldInfo, boost::pfr::tuple_size_v<S>> getNames()
{
    std::array<FieldInfo, boost::pfr::tuple_size_v<S>> array{};

    auto one = [&array]<std::size_t I>(std::integral_constant<std::size_t, I>)
    {
        constexpr std::string_view               name = boost::pfr::get_name<I, S>();
        constexpr StringLiteral<name.size() + 1> literal(name);
        if constexpr (HasTag<S, literal>)
        {
            constexpr FieldInfo info = S::jsonserMetaMethod(Tag<literal>{});
            if constexpr (info.has_value())
                array[I] = *info;
        }
        else
            array[I] = name;
    };
    auto all = [&one]<std::size_t... Is>(std::index_sequence<Is...>)
    {
        (one(std::integral_constant<std::size_t, Is>{}), ...);
    };
    all(std::make_index_sequence<boost::pfr::tuple_size_v<S>>{});
    return array;
}

enum class OnMissingValue : std::uint8_t
{
    Exception,
    DefaultFromStruct,
    DefaultFromType
};

struct DeserializeSettings
{
    OnMissingValue missingField = OnMissingValue::DefaultFromStruct;
    OnMissingValue missingOptional = OnMissingValue::DefaultFromType;
    OnMissingValue missingEnum = OnMissingValue::DefaultFromType;
};

class Deserialize
{
public:
    template <BasicJson J, typename T>
    static void fromJson(const J &j, T &v, DeserializeSettings setts = {})
    {
        // Здесь сразу 100% поле есть, лишняя проверка на contains не нужна.
        T def{};
        fromJsonImpl<J, T>(j, v, def, setts);
    }

private:
    template <typename T>
    static constexpr OnMissingValue getValueForThisType(const DeserializeSettings &setts)
    {
        if constexpr (Enum<T>)
            return setts.missingEnum;
        else if constexpr (Optional<T>)
            return setts.missingOptional;
        else
            return setts.missingField;
    }
    template <BasicJson J, typename T>
    static constexpr void fromJsonImpl(const J &j, T &v, T def, const DeserializeSettings &setts)
    {
        if constexpr (Enum<T>)
            enumFromJson<J, T>(j, v, def);
        else if constexpr (Optional<T>)
            optionalFromJson<J, T>(j, v, def, setts);
        else if constexpr (AgregatStructure<T>)
            structFromJson<J, T>(j, v, def, setts);
        else if constexpr (Deserializeable<J, T>)
            deserializableFromJson<J, T>(j, v);
        else
            throw std::logic_error("Type must be enum|optional|struct or must have from_json function.");
    }
    template <BasicJson J, typename T>
    static constexpr void fromJsonWithName(const J &j, const std::string_view sv, T &v, T &def, const DeserializeSettings &setts)
    {
        if (!j.contains(sv))
        {
            OnMissingValue valueToThis = getValueForThisType<T>(setts);
            if (valueToThis == OnMissingValue::DefaultFromType)
                v = T{};
            else if (valueToThis == OnMissingValue::DefaultFromStruct)
                v = std::move(def);
            else
                throw std::logic_error("Field " + std::string(sv) + " is missed in json.");
            return;
        }
        J jValue = j.at(sv);
        fromJsonImpl<J, T>(jValue, v, def, setts);
    }

    template <BasicJson J, Enum E>
    static constexpr void enumFromJson(const J &j, E &e, E &def)
    {
        if (!j.is_string())
            throw std::logic_error("Enum in json must be string.");
        std::string_view svValue = j.template get<std::string_view>();

        auto value = magic_enum::enum_cast<E>(svValue);
        if (!value.has_value())
            throw std::logic_error("Invalid enum value " + std::string(svValue) + '.');
        e = value.value();
    }
    template <BasicJson J, Optional O>
    static constexpr void optionalFromJson(const J &j, O &o, O &def, const DeserializeSettings &setts)
    {
        if (j.is_null())
            return (o = std::nullopt), void();

        using InternalType = typename O::value_type;
        InternalType internal{};
        fromJson(j, internal, setts); // Тип есть, он не null, можем его повторно конвертировать, но уже во внутреннний тип
        o = std::move(internal);
    }
    template <BasicJson J, typename D>
    static constexpr void deserializableFromJson(const J &j, D &d)
    {
        j.get_to(d);
    }
    template <BasicJson J, AgregatStructure S>
    static constexpr void structFromJson(const J &j, S &s, S &def, const DeserializeSettings &setts)
    {
        static constexpr auto names = getNames<S>();

        if (!j.is_object())
            throw std::logic_error("Struct in json must be object.");

        auto convertOne = [&]<std::size_t I>(std::integral_constant<std::size_t, I>)
        {
            if constexpr (constexpr FieldInfo name = names[I]; name.has_value())
                fromJsonWithName<J, boost::pfr::tuple_element_t<I, S>>(j, *name, boost::pfr::get<I>(s), boost::pfr::get<I>(def), setts);
        };
        auto convertFoo = [&convertOne]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            (convertOne(std::integral_constant<std::size_t, Is>{}), ...);
        };
        convertFoo(std::make_index_sequence<boost::pfr::tuple_size_v<S>>{});
    }
};

enum class OptionalOptionalBehaviour : bool
{
    Nullopt,
    Skip
};

struct SerializeSettings
{
    OptionalOptionalBehaviour optionalNullopt = OptionalOptionalBehaviour::Skip;
};

class Serialize
{
public:
    template <BasicJson J, typename T>
    static void toJson(J &j, const T &v, SerializeSettings setts = {})
    {
        toJsonImpl<J, T>(j, v, setts);
    }

private:
    template <BasicJson J, typename T>
    static constexpr void toJsonImpl(J &j, const T &v, const SerializeSettings &setts)
    {
        if constexpr (Enum<T>)
            enumToJson<J, T>(j, v);
        else if constexpr (Variant<T>)
            variantToJson<J, T>(j, v, setts);
        else if constexpr (Optional<T>)
            optionalToJson<J, T>(j, v, setts);
        else if constexpr (AgregatStructure<T>)
            structToJson<J, T>(j, v, setts);
        else if constexpr (Serializeable<J, T>)
            serializableToJson<J, T>(j, v);
        else
            throw std::logic_error("Type must be enum|optional|struct or must have from_json function.");
    }
    template <BasicJson J, Optional O>
    static constexpr void optionalToJsonWithName(J &j, const std::string_view sv, const O &o, const SerializeSettings &setts)
    {
        if (o.has_value())
            return toJsonImpl(j[sv], o.value(), setts);
        if (setts.optionalNullopt == OptionalOptionalBehaviour::Nullopt)
            j[sv] = nullptr;
    }
    template <BasicJson J, typename T>
    static constexpr void toJsonWithName(J &j, const std::string_view sv, const T &v, const SerializeSettings &setts)
    {
        if constexpr (Optional<T>) // Особый тип
            return optionalToJsonWithName<J, T>(j, sv, v, setts);
        toJsonImpl(j[sv], v, setts);
    }

    template <BasicJson J, Variant V>
    static constexpr void variantToJson(J &j, const V &v, const SerializeSettings &setts)
    {
        // Что-то вызовется гаарантировано
        std::visit([&j, &setts]<typename T>(const T &v)
        {
            toJsonImpl(j, v, setts);
        }, v);
    }
    template <BasicJson J, Enum E>
    static constexpr void enumToJson(J &j, const E &e)
    {
        auto name = magic_enum::enum_name(e);
        if (name.empty())
            throw std::logic_error(std::string("Enum value is out of magic_enum support range. EnumType: ") + typeid(E).name() + ". EnumValue: " + std::to_string(static_cast<int>(e)));
        j = name;
    }
    template <BasicJson J, typename D>
    static constexpr void serializableToJson(J &j, const D &d)
    {
        j = d;
    }
    template <BasicJson J, Optional O>
    static constexpr void optionalToJson(J &j, const O &o, const SerializeSettings &setts)
    {
        if (!o.has_value())
            j = nullptr;
        else
            toJsonImpl(j, o.value(), setts);
    }
    template <BasicJson J, AgregatStructure S>
    static constexpr void structToJson(J &j, const S &s, const SerializeSettings &setts)
    {
        static constexpr auto names = getNames<S>();
        j = J::object();
        auto convertOne = [&]<std::size_t I>(std::integral_constant<std::size_t, I>)
        {
            if constexpr (constexpr FieldInfo name = names[I]; name.has_value())
                toJsonWithName<J, boost::pfr::tuple_element_t<I, S>>(j, *name, boost::pfr::get<I>(s), setts);
        };
        auto convertFoo = [&convertOne]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            (convertOne(std::integral_constant<std::size_t, Is>{}), ...);
        };
        convertFoo(std::make_index_sequence<boost::pfr::tuple_size_v<S>>{});
    }
};

} // namespace jsonser

#ifndef JSONSER_DISABLE_STRUCT_MACRO

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

#endif

namespace nlohmann
{
template <jsonser::ReflectStruct T>
struct adl_serializer<T>
{
    static void to_json(json &j, const T &value)
    {
        jsonser::Serialize::toJson(j, value);
    }
    static void from_json(const json &j, T &value)
    {
        jsonser::Deserialize::fromJson(j, value);
    }
};

} // namespace nlohmann