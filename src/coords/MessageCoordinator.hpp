#pragma once

#include "utils/Types.hpp"
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/executor.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <tgbot/Types.h>

#include <openai/ChatsProcessor.hpp>
#include <openai/MessageSender.hpp>
#include <bot/ImageDownloader.hpp>

namespace coords
{
class MessageCoordinator
{
public:
    MessageCoordinator(asio::any_io_executor ex, openai::MessageSender &sender, openai::ChatsProcessor &processor)
        : ex_(ex), sender_(sender), processor_(processor)
    {
    }
    void onMessage(std::shared_ptr<TgBot::Message> msg)
    {
        if (msg->chat->type != TgBot::Chat::Type::Private)
            return;

        asio::co_spawn(ex_, processMessage(msg), asio::detached);

        //     if (msg->text)
        //         processor_.sendMessage(msg->chat->id, msg->text.value());
        // else processor_.sendMessage(msg->chat->id, "Сори, я сейчас понимаю только текст . . .");
    }

    utils::AsyncResult<void> processMessage(TgBot::Message::Ptr msg)
    {
        if (msg->photo)
        {
            auto fileId = getFileId(*msg->photo);
            bot::ImageDownloader;
        }

        co_return utils::empty;
    }

private:
    asio::any_io_executor ex_;

    openai::MessageSender  &sender_;
    openai::ChatsProcessor &processor_;

private:
    std::optional<std::string> getFileId(const std::vector<TgBot::PhotoSize::Ptr> &photos)
    {
        for (const auto &ph : photos)
            if (ph)
                return ph->fileId;
        return std::nullopt;
    }
};
} // namespace coords