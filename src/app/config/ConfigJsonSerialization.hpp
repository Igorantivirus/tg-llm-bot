#pragma once

#include <nlohmann/json.hpp>

#include "AccessRightsConfig.hpp"
#include "AppConfig.hpp"
#include "UrlConfig.hpp"

namespace config
{
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(UrlConfig, host, port);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AccessRights, creator, admins, personalChats, groups);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AppConfig, token, defaultModel, openAiUrl, adminsSettings);
} // namespace config