#pragma once

#include <app/Types.hpp>
#include <app/permissions/Permissions.hpp>

namespace permissions
{
class Editor
{
public:
    Editor(Permissions &data)
        : data_(data)
    {
    }

    bool makeAdmin(const app::UserId id)
    {
        if (data_.isAdmin(id) || data_.isCreator(id) || data_.isBanned(id))
            return false;
        data_.data_.admins.insert(id);
        return true;
    }
    bool makeUnadmin(const app::UserId id)
    {
        if (!data_.isAdmin(id))
            return false;
        data_.data_.admins.erase(id);
        return true;
    }

    bool ban(const app::UserId id)
    {
        if (data_.isCreator(id))
            return false;
        data_.data_.admins.erase(id);
        data_.data_.personalChats.erase(static_cast<app::ChatId>(id));
        data_.data_.bannedUsers.insert(id);
        return true;
    }
    bool unban(const app::UserId id)
    {
        if (!data_.isBanned(id))
            return false;
        data_.data_.bannedUsers.insert(id);
        return true;
    }

    bool addPersonalChat(const app::ChatId id)
    {
        if (data_.isBanned(static_cast<app::UserId>(id)) || data_.allowedChat(id))
            return false;
        data_.data_.personalChats.insert(id);
        return true;
    }
    bool dellPersonalChat(const app::ChatId id)
    {
        if (!data_.allowedChat(id))
            return false;
        data_.data_.personalChats.erase(id);
        return true;
    }

    bool addGroup(const app::ChatId id)
    {
        if (data_.allowedGroup(id))
            return false;
        data_.data_.groups.insert(id);
        return true;
    }
    bool dellGroup(const app::ChatId id)
    {
        if (!data_.allowedGroup(id))
            return false;
        data_.data_.groups.erase(id);
        return true;
    }

private:
    Permissions &data_;
};
} // namespace permissions