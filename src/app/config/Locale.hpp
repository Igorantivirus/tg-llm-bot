#pragma once

#include <string>
#include <unordered_map>

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
    std::string                                  currentModel;
    std::string                                  modelHelp;
    std::string                                  systemPromt;
    std::string                                  cleared;
    std::string                                  error;
    std::string                                  info;
    std::string                                  thinking;
    std::string                                  success;
    std::string                                  modelSetted;
    CommandErrors                                commandErrors;
    std::unordered_map<std::string, std::string> infoLocale;
};
} // namespace config