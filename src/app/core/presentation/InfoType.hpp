#pragma once

#include <cstdint>

namespace core
{
enum class InfoType : std::uint8_t
{
    None,
    WaitPrevTask,
    ModelNotSetedNoExist,
    SystemPromtDropped,
    SystemPromtChanged,
    ContextCleared
};
}