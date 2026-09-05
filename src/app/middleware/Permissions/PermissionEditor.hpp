#pragma once

#include "Permissions.hpp"
#include "app/middleware/Types.hpp"

namespace middleware
{
class PermissionEditor
{
public:
    PermissionEditor(Permissions &data)
        : data_(data)
    {
    }

    bool makeAdmin(const UserId id)
    {
        if (data_.isAdmin(id) || data_.isCreator(id) || data_.isBanned(id))
            return false;
        data_.data_.admins.insert(id);
        return true;
    }
    bool makeUnadmin(const UserId id)
    {
        if (!data_.isAdmin(id))
            return false;
        data_.data_.admins.erase(id);
        return true;
    }

    bool ban(const UserId id)
    {
        if (data_.isCreator(id))
            return false;
        data_.data_.admins.erase(id);
        data_.data_.personalChats.erase(static_cast<ChatId>(id));
        data_.data_.bannedUsers.insert(id);
        return true;
    }
    bool unban(const UserId id)
    {
        if (!data_.isBanned(id))
            return false;
        data_.data_.bannedUsers.insert(id);
        return true;
    }

    bool addPersonalChat(const ChatId id)
    {
        if (data_.isBanned(static_cast<UserId>(id)) || data_.allowedChat(id))
            return false;
        data_.data_.personalChats.insert(id);
        return true;
    }
    bool dellPersonalChat(const ChatId id)
    {
        if (!data_.allowedChat(id))
            return false;
        data_.data_.personalChats.erase(id);
        return true;
    }

    bool addGroup(const ChatId id)
    {
        if (data_.allowedGroup(id))
            return false;
        data_.data_.groups.insert(id);
        return true;
    }
    bool dellGroup(const ChatId id)
    {
        if (!data_.allowedGroup(id))
            return false;
        data_.data_.groups.erase(id);
        return true;
    }

private:
    Permissions &data_;
};
} // namespace middleware