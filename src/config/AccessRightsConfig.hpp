#pragma once

#include <vector>

#include <openaiprocessor/Types.hpp>

namespace config
{

struct AccessRights
{
    ChatID creator;
    std::vector<ChatID> admins;
    std::vector<ChatID> personalChats;
    std::vector<ChatID> groups;
};

} // namespace config