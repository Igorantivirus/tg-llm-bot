#pragma once

#include "UrlConfig.hpp"
#include <string>

namespace config
{

struct AppConfig
{
    std::string  token;
    std::string  defaultModel;
    UrlConfig    openAiUrl;
    std::string  accessRightsFile;
    std::uint8_t threadCount;
    std::uint8_t tcpSocketsCount;
};

} // namespace config