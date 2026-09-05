#pragma once

#include <unordered_set>

#include "PermissionData.hpp"
#include <app/middleware/Types.hpp>

namespace middleware
{
class PermissionEditor;
class PermissionReadWriter;
class Permissions
{
    friend class PermissionEditor;
    friend class PermissionReadWriter;

public:
    // Создатель
    inline bool isCreator(const UserId id) const
    {
        return id == data_.creator;
    }
    // Админ
    inline bool isAdmin(const UserId id) const
    {
        return data_.admins.contains(id);
    }
    // Забаненый
    inline bool isBanned(const UserId id) const
    {
        return data_.bannedUsers.contains(id);
    }
    // Допустимый личный чат
    bool allowedChat(const ChatId id) const
    {
        return data_.personalChats.contains(id) && !isBanned(id); // id персонального чата совпадает с id пользователя. Забаненый пользователь не может общаться в лс
    }
    // Допустимая группа
    bool allowedGroup(const ChatId id) const
    {
        return data_.groups.contains(id);
    }

private:
    PermissionData data_;
};
} // namespace middleware