#pragma once

#include <app/permissions/Permissions.hpp>
#include <tgbot/Types.h>

namespace bot
{
class PermissionChecker
{
public:
    PermissionChecker(permissions::Permissions &data)
        : data_(data)
    {
    }

    bool checkBaseCommand(const TgBot::Chat::Type type, const app::ChatId chat, const app::UserId user)
    {
        if (data_.isCreator(user)) // Создателю можно всё
            return true;
        if (type == TgBot::Chat::Type::Private) // В личном чате проверяем по доступу к чату
            return data_.allowedChat(chat);
        return data_.allowedGroup(chat) && !data_.isBanned(user) && data_.isAdmin(user);
    }
    bool checkPermissionCommand(const TgBot::Chat::Type type, const app::ChatId chat, const app::UserId user)
    {
        return data_.isCreator(user);
    }
    bool checkMessage(const TgBot::Chat::Type type, const app::ChatId chat, const app::UserId user)
    {
        if (data_.isCreator(user)) // Создателю можно всё
            return true;
        if (type == TgBot::Chat::Type::Private) // В личном чате проверяем по доступу к чату
            return data_.allowedChat(chat);
        return data_.allowedGroup(chat) && !data_.isBanned(user);
    }

private:
    permissions::Permissions &data_;
};
} // namespace bot