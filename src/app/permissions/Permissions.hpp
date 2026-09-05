#pragma once

#include <unordered_set>

#include <app/Types.hpp>
#include <app/permissions/Data.hpp>

namespace permissions
{
class Editor;
class ReadWriter;
class Permissions
{
    friend class Editor;
    friend class ReadWriter;

public:
    // Создатель
    inline bool isCreator(const app::UserId id) const
    {
        return id == data_.creator;
    }
    // Админ
    inline bool isAdmin(const app::UserId id) const
    {
        return data_.admins.contains(id);
    }
    // Забаненый
    inline bool isBanned(const app::UserId id) const
    {
        return data_.bannedUsers.contains(id);
    }
    // Допустимый личный чат
    bool allowedChat(const app::ChatId id) const
    {
        return data_.personalChats.contains(id) && !isBanned(id); // id персонального чата совпадает с id пользователя. Забаненый пользователь не может общаться в лс
    }
    // Допустимая группа
    bool allowedGroup(const app::ChatId id) const
    {
        return data_.groups.contains(id);
    }

private:
    Data data_;
};
} // namespace permissions