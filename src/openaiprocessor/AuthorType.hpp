#pragma once

#include <cstdint>
#include <string_view>

enum class AuthorType : std::uint8_t
{
    User,
    Assistent
};

constexpr const std::string_view toString(const AuthorType type)
{
    constexpr static const std::string_view values[] = {"user", "assistent"};
    const std::uint8_t index = static_cast<std::uint8_t>(type);
    return index > 1 ? "" : values[index];
}