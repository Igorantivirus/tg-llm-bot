#pragma once

#include <string>

namespace config
{
struct CommandErrors
{
    std::string permissionError;
    std::string argumentsError;
    std::string invalidUserId;
    std::string commandError;
};
struct Locale
{
    std::string   currentModel;
    std::string   systemPromt;
    std::string   cleared;
    std::string   error;
    std::string   info;
    std::string   thinking;
    CommandErrors commandErrors;
};
} // namespace config