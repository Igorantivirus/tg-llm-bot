#pragma once

#include <nlohmann/json.hpp>

#include "AppConfig.hpp"

namespace config
{
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AppConfig, token);
}