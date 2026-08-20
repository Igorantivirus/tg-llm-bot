#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>  // ДОБАВЛЕНО: поддержка std::optional-полей
#include <stdexcept> // ДОБАВЛЕНО: базовый класс для jsonser::Error
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <boost/pfr/core.hpp>
#include <boost/pfr/core_name.hpp>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>

// ДОБАВЛЕНО: поиск/индексация по std::string_view появились только в 3.11.
// Без этого на старой версии получаем портянку ошибок вместо внятного сообщения.
static_assert(NLOHMANN_JSON_VERSION_MAJOR > 3 || (NLOHMANN_JSON_VERSION_MAJOR == 3 && NLOHMANN_JSON_VERSION_MINOR >= 11),
              "utils::jsonser requires nlohmann/json >= 3.11");

namespace utils::jsonser
{

// ============================================================================
//  ОШИБКИ
// ============================================================================

// ДОБАВЛЕНО: единый тип исключения.
// Раньше коды nlohmann использовались не по назначению (501 в nlohmann означает
// провал теста JSON Patch, 403 — отсутствие ключа при обращении по указателю),
// а из-за этого сообщения вводили в заблуждение. Наследоваться от
// nlohmann::json::exception нельзя (конструкторы закрыты), поэтому свой тип.
// Все исключения, вылетающие из jsonser, — это Error; исключения nlohmann,
// прилетевшие из вложенных типов, оборачиваются в Error с указанием поля.
class Error : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

// ============================================================================
//  ПАРАМЕТРЫ ДЕСЕРИАЛИЗАЦИИ
// ============================================================================

// ИЗМЕНЕНО: вместо голого bool UseDefaults — именованная политика.
enum class MissingPolicy : std::uint8_t
{
    Throw,     // отсутствующее поле — ошибка
    UseDefault // отсутствующее поле остаётся со значением из Type{} (в т.ч. NSDMI)
};

// ДОБАВЛЕНО: набор опций одним NTTP-параметром — расширяется без ломки сигнатур.
struct Options
{
    MissingPolicy missing = MissingPolicy::Throw;
    bool          rejectUnknownKeys = false; // ДОБАВЛЕНО: строгий режим — ловит опечатки в конфигах
    bool          treatNullAsMissing = true; // null трактуется как отсутствие ключа
};

inline constexpr Options kStrict{.missing = MissingPolicy::Throw};
inline constexpr Options kWithDefaults{.missing = MissingPolicy::UseDefault};

// ============================================================================
//  КОМПАЙЛ-ТАЙМ СТРОКИ
// ============================================================================

template <std::size_t N>
struct StringLiteral
{
    constexpr StringLiteral() = default;
    // ИЗМЕНЕНО: constexpr вместо consteval — иначе литерал нельзя построить
    // внутри другой constexpr-функции на некоторых компиляторах.
    constexpr StringLiteral(const char (&str)[N])
    {
        std::copy_n(str, N, string);
    }
    [[nodiscard]] constexpr std::size_t size() const
    {
        return N - 1;
    }
    [[nodiscard]] constexpr std::string_view view() const // ДОБАВЛЕНО: явный метод
    {
        return std::string_view(string, N - 1);
    }
    constexpr operator std::string_view() const
    {
        return view();
    }

    char string[N]{}; // public — обязательное условие структурного типа для NTTP
};

template <std::size_t N>
StringLiteral(const char (&)[N]) -> StringLiteral<N>;

template <StringLiteral S>
struct Tag
{
    static constexpr std::string_view          view = S.view(); // ДОБАВЛЕНО: нужно для JSONSER_FDEFN
    [[nodiscard]] consteval static std::size_t size()
    {
        return S.size();
    }
};

// ИЗМЕНЕНО: Info -> FieldInfo (понятнее, и имя Info слишком общее для namespace-scope)
struct FieldInfo
{
    std::string_view name{};
    bool             skip = false;
};

// ============================================================================
//  ПРОВЕРКИ ТИПОВ
// ============================================================================

// ДОБАВЛЕНО: конвертеры теперь шаблонны по типу json — работают и с
// nlohmann::json, и с nlohmann::ordered_json, и с любым другим basic_json.
template <typename J>
concept BasicJson = requires {
    typename std::remove_cvref_t<J>::object_t;
    typename std::remove_cvref_t<J>::string_t;
    typename std::remove_cvref_t<J>::exception;
};

namespace detail
{

template <typename T>
struct IsOptionalImpl : std::false_type
{
};
template <typename T>
struct IsOptionalImpl<std::optional<T>> : std::true_type
{
};
template <typename T>
inline constexpr bool isOptional = IsOptionalImpl<std::remove_cvref_t<T>>::value;

// ------------------------------------------------------------------
//  Разбор объявления поля на этапе компиляции
// ------------------------------------------------------------------
// ДОБАВЛЕНО: макрос теперь получает объявление целиком ("std::uint64_t id = 0"),
// а имя поля ("id") вырезается здесь. Благодаря этому работают типы с запятыми
// внутри (std::map<int, int>), инициализаторы по умолчанию и массивы.

consteval bool isIdentChar(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

// Конец декларатора: до инициализатора и до размерности массива.
consteval std::size_t declNameEnd(std::string_view decl)
{
    std::size_t end = decl.size();
    for (std::size_t i = 0; i < decl.size(); ++i)
        if (decl[i] == '=' || decl[i] == '{' || decl[i] == '[')
        {
            end = i;
            break;
        }
    while (end > 0 && !isIdentChar(decl[end - 1]))
        --end;
    return end;
}

consteval std::size_t declNameBegin(std::string_view decl, std::size_t end)
{
    std::size_t begin = end;
    while (begin > 0 && isIdentChar(decl[begin - 1]))
        --begin;
    return begin;
}

template <std::size_t Len>
consteval auto toLiteral(std::string_view sv)
{
    // ДОБАВЛЕНО: в consteval-контексте это превращается в ошибку компиляции,
    // а не в тихий выход за границы, как было раньше.
    if (sv.size() != Len)
        throw "toLiteral: length mismatch";
    StringLiteral<Len + 1> out{};
    for (std::size_t i = 0; i < Len; ++i)
        out.string[i] = sv[i];
    return out;
}

template <StringLiteral Decl>
consteval auto fieldNameOf()
{
    constexpr std::string_view decl = Decl.view();
    constexpr std::size_t      end = declNameEnd(decl);
    constexpr std::size_t      begin = declNameBegin(decl, end);
    static_assert(end > begin, "Cannot extract field name from the declaration passed to a JSONSER_* macro.");
    return toLiteral<end - begin>(decl.substr(begin, end - begin));
}

// ИЗМЕНЕНО: HasJsonMetaOverload -> HasFieldMeta, метод переименован в jsonserFieldMeta
template <StringLiteral literal, typename Type>
concept HasFieldMeta = requires {
    { Type::jsonserFieldMeta(Tag<literal>{}) } -> std::same_as<FieldInfo>;
};

// ------------------------------------------------------------------
//  Источники имён полей
// ------------------------------------------------------------------

// ДОБАВЛЕНО: политика именования. Позволяет иметь один общий движок
// сериализации агрегатов для JsonConverter и JsonSimpleConverter.
struct MetaNaming
{
    template <std::size_t I, typename Type>
    consteval static FieldInfo info()
    {
        constexpr auto name = boost::pfr::get_name<I, Type>();
        constexpr auto literal = toLiteral<name.size()>(name);
        static_assert(HasFieldMeta<literal, Type>,
                      "Every field must be declared via JSONSER_FIELD / JSONSER_FDEFN / JSONSER_FSKIP.");
        return Type::jsonserFieldMeta(Tag<literal>{});
    }
};

struct PlainNaming
{
    template <std::size_t I, typename Type>
    consteval static FieldInfo info()
    {
        return FieldInfo{.name = boost::pfr::get_name<I, Type>(), .skip = false};
    }
};

template <typename Naming, typename Type>
consteval auto collectFieldInfos()
{
    return []<std::size_t... Is>(std::index_sequence<Is...>)
    {
        return std::array<FieldInfo, sizeof...(Is)>{Naming::template info<Is, Type>()...};
    }(std::make_index_sequence<boost::pfr::tuple_size_v<Type>>{});
}

// ИЗМЕНЕНО: принимает готовый массив вместо index_sequence — метаинформация
// считается один раз на тип, а не заново в каждой функции (время компиляции).
template <std::size_t N>
consteval bool hasNoDuplicates(const std::array<FieldInfo, N> &infos)
{
    for (std::size_t i = 0; i < N; ++i)
    {
        if (infos[i].skip)
            continue;
        for (std::size_t j = i + 1; j < N; ++j)
            if (!infos[j].skip && infos[i].name == infos[j].name)
                return false;
    }
    return true;
}

template <std::size_t N>
consteval bool hasNoEmptyNames(const std::array<FieldInfo, N> &infos)
{
    for (const FieldInfo &i : infos)
        if (!i.skip && i.name.empty())
            return false;
    return true;
}

// ------------------------------------------------------------------
//  Работа с одним полем
// ------------------------------------------------------------------

inline std::string describe(const char *typeName, std::string_view field)
{
    return "field '" + std::string(field) + "' of type '" + std::string(typeName) + "'";
}

// ДОБАВЛЕНО: контекст ошибки. Раньше исключение из вложенного типа не содержало
// ни имени поля, ни имени структуры — отладка трёхуровневого json была гаданием.
template <BasicJson J, typename Field>
void readValue(const J &value, Field &out, const char *typeName, std::string_view field)
{
    try
    {
        value.get_to(out); // ИЗМЕНЕНО: get_to вместо get<T>() — без лишней копии
    }
    catch (const std::exception &e)
    {
        throw Error("cannot deserialize " + describe(typeName, field) + ": " + e.what());
    }
}

template <Options Opts, BasicJson J, typename Field>
void readField(const J &j, Field &out, const char *typeName, std::string_view name)
{
    const auto found = j.find(name);
    const bool absent = (found == j.end());
    const bool null = !absent && found->is_null();
    const bool missing = absent || (Opts.treatNullAsMissing && null);

    if constexpr (isOptional<Field>)
    {
        // ДОБАВЛЕНО: optional-поле отсутствует -> nullopt, а не ошибка.
        if (missing)
        {
            out.reset();
            return;
        }
        out.emplace();
        readValue(*found, *out, typeName, name);
        return;
    }
    else
    {
        if (missing)
        {
            if constexpr (Opts.missing == MissingPolicy::UseDefault)
                return; // оставляем значение из Type{} — см. fromJson
            // ИЗМЕНЕНО: раньше "key not found" врало, когда ключ есть, но равен null.
            throw Error(absent ? "cannot deserialize " + describe(typeName, name) + ": key is missing"
                               : "cannot deserialize " + describe(typeName, name) + ": key is null");
        }
        readValue(*found, out, typeName, name);
    }
}

template <BasicJson J, typename Field>
void writeField(J &j, const Field &value, const char *typeName, std::string_view name)
{
    try
    {
        if constexpr (isOptional<Field>)
        {
            // ДОБАВЛЕНО: пустой optional не пишется в json вовсе.
            if (value.has_value())
                j[name] = *value;
        }
        else
        {
            j[name] = value;
        }
    }
    catch (const std::exception &e)
    {
        throw Error("cannot serialize " + describe(typeName, name) + ": " + e.what());
    }
}

// ------------------------------------------------------------------
//  Общий движок для агрегатов
// ------------------------------------------------------------------

template <typename Naming, typename Type>
struct AggregateConverter
{
    static_assert(std::is_aggregate_v<Type>, "Type must be an aggregate.");
    // ДОБАВЛЕНО: одного is_aggregate_v мало — boost::pfr не поддерживает базовые
    // классы и полиморфные типы, а диагностика уезжает вглубь PFR.
    static_assert(!std::is_polymorphic_v<Type>, "Polymorphic types are not reflectable.");
    static_assert(!std::is_union_v<Type>, "Unions are not reflectable.");
    // ДОБАВЛЕНО: нужно и для дефолтов, и для строгой гарантии исключений.
    static_assert(std::is_default_constructible_v<Type>, "Type must be default constructible.");
    static_assert(std::is_move_assignable_v<Type>, "Type must be move assignable.");

    static constexpr std::size_t fieldCount = boost::pfr::tuple_size_v<Type>;
    static constexpr auto        infos = collectFieldInfos<Naming, Type>();

    static_assert(hasNoEmptyNames(infos), "Json field name must not be empty.");
    static_assert(hasNoDuplicates(infos), "Type has duplicate json field names.");

    // ИЗМЕНЕНО: constexpr снят — внутри аллокации и nlohmann::json,
    // вычислить это на этапе компиляции невозможно, обещание было ложным.
    template <BasicJson J>
    static void toJson(J &j, const Type &v, const char *typeName)
    {
        // ИЗМЕНЕНО: гарантируем объект. Раньше структура без сериализуемых полей
        // давала null, а непустой j домешивал поля к чужим данным.
        j = J::object();

        const auto writeOne = [&]<std::size_t I>(std::integral_constant<std::size_t, I>)
        {
            constexpr FieldInfo info = infos[I];
            if constexpr (!info.skip)
                writeField(j, boost::pfr::get<I>(v), typeName, info.name);
        };
        [&]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            (writeOne(std::integral_constant<std::size_t, Is>{}), ...);
        }(std::make_index_sequence<fieldCount>{});
    }

    template <Options Opts = kStrict, BasicJson J>
    static void fromJson(const J &j, Type &v, const char *typeName)
    {
        // ИЗМЕНЕНО: был out_of_range/403 с сообщением про ключ; тип ошибки не тот.
        if (!j.is_object())
            throw Error("cannot deserialize '" + std::string(typeName) + "': expected object, got " + j.type_name());

        if constexpr (Opts.rejectUnknownKeys)
            rejectUnknown(j, typeName);

        // ИЗМЕНЕНО: заполняем временный объект.
        // 1) строгая гарантия исключений — при ошибке v не остаётся полузаполненным;
        // 2) "дефолт" = значение из Type{}, т.е. из инициализатора члена (NSDMI),
        //    а не жёсткий ноль, как было в enum-ветке.
        Type tmp{};

        const auto readOne = [&]<std::size_t I>(std::integral_constant<std::size_t, I>)
        {
            constexpr FieldInfo info = infos[I];
            if constexpr (!info.skip)
                readField<Opts>(j, boost::pfr::get<I>(tmp), typeName, info.name);
        };
        [&]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            (readOne(std::integral_constant<std::size_t, Is>{}), ...);
        }(std::make_index_sequence<fieldCount>{});

        v = std::move(tmp);
    }

private:
    template <BasicJson J>
    static void rejectUnknown(const J &j, const char *typeName)
    {
        for (const auto &item : j.items())
        {
            const bool known = std::any_of(infos.begin(), infos.end(), [&](const FieldInfo &i)
            {
                return !i.skip && i.name == item.key();
            });
            if (!known)
                throw Error("cannot deserialize '" + std::string(typeName) + "': unknown key '" + item.key() + "'");
        }
    }
};

} // namespace detail

// ============================================================================
//  КОНВЕРТАЦИЯ С ПЕРЕИМЕНОВАНИЕМ ПОЛЕЙ (макросы)
// ============================================================================

template <typename T, typename Enable = void>
struct JsonConverter;

template <typename Enum>
struct JsonConverter<Enum, std::enable_if_t<std::is_enum_v<Enum>>>
{
    template <BasicJson J>
    static void toJson(J &j, const Enum &v, const char *typeName)
    {
        const std::string_view str = magic_enum::enum_name(v);
        if (str.empty())
            throw Error("cannot serialize '" + std::string(typeName) + "': value " +
                        std::to_string(static_cast<std::underlying_type_t<Enum>>(v)) + " is not a named enumerator");
        // ИЗМЕНЕНО: явное построение string_t вместо неявной конверсии string_view.
        j = typename J::string_t(str);
    }

    template <Options Opts = kStrict, BasicJson J>
    static void fromJson(const J &j, Enum &v, const char *typeName)
    {
        if (!j.is_string())
            throw Error("cannot deserialize '" + std::string(typeName) + "': expected string, got " + j.type_name());

        const auto &s = j.template get_ref<const typename J::string_t &>();
        // ИСПРАВЛЕН БАГ: в старой версии при UseDefaults выполнялось v = Enum{},
        // после чего безусловно шло v = *value на пустом optional — UB.
        if (const auto value = magic_enum::enum_cast<Enum>(std::string_view(s)); value.has_value())
        {
            v = *value;
            return;
        }
        if constexpr (Opts.missing == MissingPolicy::UseDefault)
            return; // ИЗМЕНЕНО: оставляем текущее значение вместо Enum{} —
                    // Enum{} (то есть 0) не обязан быть валидным энумератором.
        throw Error("cannot deserialize '" + std::string(typeName) + "': '" + s + "' is not a valid enumerator");
    }
};

template <typename Type>
struct JsonConverter<Type, std::enable_if_t<!std::is_enum_v<Type> && std::is_class_v<Type>>>
    : detail::AggregateConverter<detail::MetaNaming, Type>
{
};

// ============================================================================
//  КОНВЕРТАЦИЯ НАПРЯМУЮ ПО ИМЕНАМ ПОЛЕЙ C++
// ============================================================================

template <typename T, typename Enable = void>
struct JsonSimpleConverter;

// enum: полностью переиспользуем готовое (наследование, а не alias —
// alias-шаблон нельзя частично специализировать).
template <typename Enum>
struct JsonSimpleConverter<Enum, std::enable_if_t<std::is_enum_v<Enum>>>
    : JsonConverter<Enum, std::enable_if_t<std::is_enum_v<Enum>>>
{
};

template <typename Type>
struct JsonSimpleConverter<Type, std::enable_if_t<!std::is_enum_v<Type> && std::is_class_v<Type>>>
    : detail::AggregateConverter<detail::PlainNaming, Type>
{
};

} // namespace utils::jsonser

// ============================================================================
//  МАКРОСЫ
// ============================================================================

#ifndef JSONSER_DISABLE_MACROS

// ДОБАВЛЕНО: подсказка для сборок со старым препроцессором MSVC.
#if defined(_MSC_VER) && !defined(__clang__) && defined(_MSVC_TRADITIONAL) && _MSVC_TRADITIONAL
#pragma message("utils::jsonser: recommended to build with /Zc:preprocessor")
#endif

// ---------------------------------------------------------------------------
//  ДОБАВЛЕНО: препроцессорные хелперы.
//  Нужны, чтобы объявление поля шло ПЕРВЫМ, а json-имя — последним.
//  Вариадическая часть обязана быть последней, поэтому забираем всё в `...`
//  и делим сами: JSONSER_LAST — json-имя, JSONSER_INIT — объявление,
//  склеенное обратно через запятые (это восстанавливает типы вида
//  std::map<int, int>, которые препроцессор разрезал на несколько аргументов).
//
//  ВНИМАНИЕ MSVC: рекомендуется /Zc:preprocessor (стандартный препроцессор).
//  ДОБАВЛЕНО: JSONSER_EXPAND — лишний слой разворачивания. Старый препроцессор
//  MSVC передаёт __VA_ARGS__ вложенному макросу как ОДИН токен, из-за чего
//  подсчёт аргументов ломается; дополнительный проход это чинит.
// ---------------------------------------------------------------------------
#define JSONSER_EXPAND(...) __VA_ARGS__

#define JSONSER_CAT_(a, b) a##b
#define JSONSER_CAT(a, b) JSONSER_CAT_(a, b)

// Стрингификация с поддержкой запятых внутри.
#define JSONSER_STR_(...) #__VA_ARGS__
#define JSONSER_STR(...) JSONSER_EXPAND(JSONSER_STR_(__VA_ARGS__))

#define JSONSER_NARG(...) \
    JSONSER_EXPAND(JSONSER_NARG_(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1))
#define JSONSER_NARG_(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, N, ...) N

#define JSONSER_LAST_1(a1) a1
#define JSONSER_LAST_2(a1, ...) JSONSER_EXPAND(JSONSER_LAST_1(__VA_ARGS__))
#define JSONSER_LAST_3(a1, ...) JSONSER_EXPAND(JSONSER_LAST_2(__VA_ARGS__))
#define JSONSER_LAST_4(a1, ...) JSONSER_EXPAND(JSONSER_LAST_3(__VA_ARGS__))
#define JSONSER_LAST_5(a1, ...) JSONSER_EXPAND(JSONSER_LAST_4(__VA_ARGS__))
#define JSONSER_LAST_6(a1, ...) JSONSER_EXPAND(JSONSER_LAST_5(__VA_ARGS__))
#define JSONSER_LAST_7(a1, ...) JSONSER_EXPAND(JSONSER_LAST_6(__VA_ARGS__))
#define JSONSER_LAST_8(a1, ...) JSONSER_EXPAND(JSONSER_LAST_7(__VA_ARGS__))
#define JSONSER_LAST_9(a1, ...) JSONSER_EXPAND(JSONSER_LAST_8(__VA_ARGS__))
#define JSONSER_LAST_10(a1, ...) JSONSER_EXPAND(JSONSER_LAST_9(__VA_ARGS__))
#define JSONSER_LAST_11(a1, ...) JSONSER_EXPAND(JSONSER_LAST_10(__VA_ARGS__))
#define JSONSER_LAST_12(a1, ...) JSONSER_EXPAND(JSONSER_LAST_11(__VA_ARGS__))
#define JSONSER_LAST_13(a1, ...) JSONSER_EXPAND(JSONSER_LAST_12(__VA_ARGS__))
#define JSONSER_LAST_14(a1, ...) JSONSER_EXPAND(JSONSER_LAST_13(__VA_ARGS__))
#define JSONSER_LAST_15(a1, ...) JSONSER_EXPAND(JSONSER_LAST_14(__VA_ARGS__))
#define JSONSER_LAST_16(a1, ...) JSONSER_EXPAND(JSONSER_LAST_15(__VA_ARGS__))
#define JSONSER_LAST(...) \
    JSONSER_EXPAND(JSONSER_CAT(JSONSER_LAST_, JSONSER_NARG(__VA_ARGS__))(__VA_ARGS__))

// Один аргумент -> json-имя не передали: имя ниже не объявлено, поэтому
// компилятор выдаст сообщение с понятным текстом.
#define JSONSER_INIT_1(a1) JSONSER_FIELD_REQUIRES_A_JSON_NAME_AS_THE_LAST_ARGUMENT
#define JSONSER_INIT_2(a1, a2) a1
#define JSONSER_INIT_3(a1, ...) a1, JSONSER_EXPAND(JSONSER_INIT_2(__VA_ARGS__))
#define JSONSER_INIT_4(a1, ...) a1, JSONSER_EXPAND(JSONSER_INIT_3(__VA_ARGS__))
#define JSONSER_INIT_5(a1, ...) a1, JSONSER_EXPAND(JSONSER_INIT_4(__VA_ARGS__))
#define JSONSER_INIT_6(a1, ...) a1, JSONSER_EXPAND(JSONSER_INIT_5(__VA_ARGS__))
#define JSONSER_INIT_7(a1, ...) a1, JSONSER_EXPAND(JSONSER_INIT_6(__VA_ARGS__))
#define JSONSER_INIT_8(a1, ...) a1, JSONSER_EXPAND(JSONSER_INIT_7(__VA_ARGS__))
#define JSONSER_INIT_9(a1, ...) a1, JSONSER_EXPAND(JSONSER_INIT_8(__VA_ARGS__))
#define JSONSER_INIT_10(a1, ...) a1, JSONSER_EXPAND(JSONSER_INIT_9(__VA_ARGS__))
#define JSONSER_INIT_11(a1, ...) a1, JSONSER_EXPAND(JSONSER_INIT_10(__VA_ARGS__))
#define JSONSER_INIT_12(a1, ...) a1, JSONSER_EXPAND(JSONSER_INIT_11(__VA_ARGS__))
#define JSONSER_INIT_13(a1, ...) a1, JSONSER_EXPAND(JSONSER_INIT_12(__VA_ARGS__))
#define JSONSER_INIT_14(a1, ...) a1, JSONSER_EXPAND(JSONSER_INIT_13(__VA_ARGS__))
#define JSONSER_INIT_15(a1, ...) a1, JSONSER_EXPAND(JSONSER_INIT_14(__VA_ARGS__))
#define JSONSER_INIT_16(a1, ...) a1, JSONSER_EXPAND(JSONSER_INIT_15(__VA_ARGS__))
#define JSONSER_INIT(...) \
    JSONSER_EXPAND(JSONSER_CAT(JSONSER_INIT_, JSONSER_NARG(__VA_ARGS__))(__VA_ARGS__))

// Внутренний хелпер, не для прямого использования.
// ИЗМЕНЕНО: static_assert теперь проверяет json-имя, а не имя поля C++
// (последнее не может быть пустым в принципе, проверка была бессмысленной).
#define JSONSER_META_IMPL(DECL, JSON_NAME, SKIP)                                                          \
    consteval static ::utils::jsonser::FieldInfo jsonserFieldMeta(                                        \
        ::utils::jsonser::Tag<::utils::jsonser::detail::fieldNameOf<DECL>()> jsonserTag [[maybe_unused]]) \
    {                                                                                                     \
        return ::utils::jsonser::FieldInfo{.name = JSON_NAME, .skip = SKIP};                              \
    }

// ИЗМЕНЕНО: порядок аргументов — сначала объявление, потом json-имя.
// Поле с явным json-именем: JSONSER_FIELD(std::uint64_t id = 0, "user_id");
#define JSONSER_FIELD(...)     \
    JSONSER_INIT(__VA_ARGS__); \
    JSONSER_META_IMPL(JSONSER_STR(JSONSER_INIT(__VA_ARGS__)), JSONSER_LAST(__VA_ARGS__), false)

// Поле, json-имя совпадает с именем в C++: JSONSER_FDEFN(std::string name);
#define JSONSER_FDEFN(...) \
    __VA_ARGS__;           \
    JSONSER_META_IMPL(#__VA_ARGS__, decltype(jsonserTag)::view, false)

// Поле, не участвующее в сериализации: JSONSER_FSKIP(int cache = 0);
#define JSONSER_FSKIP(...) \
    __VA_ARGS__;           \
    JSONSER_META_IMPL(#__VA_ARGS__, {}, true)

// ИЗМЕНЕНО: to_json/from_json стали шаблонами по типу json (ordered_json и др.)
// и ограничены концептом, чтобы не перехватывать чужие перегрузки через ADL.
// Макрос должен вызываться в том же namespace, что и TYPE, — иначе ADL их не найдёт.
#define JSONSER_OUTLINE_EX(TYPE, CONVERTER, ...)                                            \
    template <::utils::jsonser::BasicJson JsonserJson>                                      \
    void to_json(JsonserJson &j, const TYPE &value)                                         \
    {                                                                                       \
        ::utils::jsonser::CONVERTER<TYPE>::toJson(j, value, #TYPE);                         \
    }                                                                                       \
    template <::utils::jsonser::BasicJson JsonserJson>                                      \
    void from_json(const JsonserJson &j, TYPE &value)                                       \
    {                                                                                       \
        ::utils::jsonser::CONVERTER<TYPE>::template fromJson<__VA_ARGS__>(j, value, #TYPE); \
    }

#define JSONSER_SERIALIZATION_OUTLINE(TYPE) \
    JSONSER_OUTLINE_EX(TYPE, JsonConverter, ::utils::jsonser::kStrict)

#define JSONSER_SERIALIZATION_OUTLINE_WITH_DEFAULTS(TYPE) \
    JSONSER_OUTLINE_EX(TYPE, JsonConverter, ::utils::jsonser::kWithDefaults)

// ИЗМЕНЕНО: было JSONSER__SIMPLE_... — двойное подчёркивание в идентификаторах
// зарезервировано за реализацией.
#define JSONSER_SIMPLE_SERIALIZATION_OUTLINE(TYPE) \
    JSONSER_OUTLINE_EX(TYPE, JsonSimpleConverter, ::utils::jsonser::kStrict)

#define JSONSER_SIMPLE_SERIALIZATION_OUTLINE_WITH_DEFAULTS(TYPE) \
    JSONSER_OUTLINE_EX(TYPE, JsonSimpleConverter, ::utils::jsonser::kWithDefaults)

#endif // JSONSER_DISABLE_MACROS