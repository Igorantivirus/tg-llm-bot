#pragma once

#include <unordered_set>

#include <app/middleware/Types.hpp>

namespace middleware
{
struct PermissionData
{
    std::int64_t                     creator{};     // Может всё
    std::unordered_set<std::int64_t> admins;        // Может писать команды (не меняющие разрешения)
    std::unordered_set<std::int64_t> bannedUsers;   // Не могут ничего, игнорируются
    std::unordered_set<std::int64_t> personalChats; // Личные чаты, где разрешено общаться (id пользователей)
    std::unordered_set<std::int64_t> groups;        // Группы, где разрешено общаться
};
} // namespace middleware