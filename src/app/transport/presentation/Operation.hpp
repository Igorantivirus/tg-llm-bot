#pragma once

#include <cstdint>
#include <string>

namespace transport
{
enum class OperationType : std::uint8_t
{
    SetMdl, // Установить модель
    SetEfr, // Установит effort
    Close,  // Закрыть панель
};

struct Operation
{
    OperationType type;
    std::string   data;
};
} // namespace transport