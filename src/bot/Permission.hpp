#pragma once

#include <cstdint>

namespace bot
{
enum class Permission : std::uint8_t
{
    User,
    ChatBotAdmin,
    FullBotAdmin,
    Creator
};
}