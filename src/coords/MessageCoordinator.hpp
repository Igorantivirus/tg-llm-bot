#pragma once

#include <tgbot/Types.h>

#include <openai/ChatsProcessor.hpp>
#include <openai/MessageSender.hpp>

namespace coords
{
class MessageCoordinator
{
public:
    MessageCoordinator(openai::MessageSender &sender, openai::ChatsProcessor &processor)
        : sender_(sender), processor_(processor)
    {
    }
    void onMessage(std::shared_ptr<TgBot::Message> msg)
    {
        if (msg->chat->type != TgBot::Chat::Type::Private)
            return;

        if (msg->text)
            processor_.sendMessage(msg->chat->id, msg->text.value());
        else
            processor_.sendMessage(msg->chat->id, "Сори, я сейчас понимаю только текст . . .");
    }

private:
    openai::MessageSender  &sender_;
    openai::ChatsProcessor &processor_;
};
} // namespace coords