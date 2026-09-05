#pragma once

#include <cstdint>
#include <string>

namespace transport
{
enum class OperationType : std::uint8_t
{
    SetMdl // Установить модель
};

struct Operation
{
    OperationType type;
    std::string   data;
};
} // namespace transport