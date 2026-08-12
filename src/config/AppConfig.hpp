#pragma once

#include "AccessRightsConfig.hpp"
#include "UrlConfig.hpp"
#include <string>

namespace config
{

struct AppConfig
{
    std::string token;
    std::string defaultModel;
    UrlConfig openAiUrl;
    AccessRights adminsSettings;
};

} // namespace config