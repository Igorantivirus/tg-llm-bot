#pragma once

#include "app/transport/TgBotMessageSender.hpp"
#include "openai/chatssettings/AdditionalsToMessage.hpp"
#include "openai/chatssettings/HistoryUtils.hpp"
#include "openai/chatssettings/Types.hpp"
#include "utils/Format.hpp"
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <app/core/Operator.hpp>
#include <app/store/ImageStore.hpp>
#include <tgbot/Types.h>
#include <utils/Base64.hpp>

namespace handlers
{

class MessagesProcessor
{
    using MessagesHandler = asio::awaitable<void> (MessagesProcessor::*)(std::vector<TgBot::Message::Ptr>);
    class MessageCollector : public std::enable_shared_from_this<MessageCollector>
    {
    public:
        using Ptr = std::shared_ptr<MessageCollector>;

        MessageCollector(asio::any_io_executor ex, std::chrono::milliseconds timeout, MessagesProcessor *proc, MessagesHandler handler, std::string groupId)
            : ex_(std::move(ex)), timer_(ex_), timeout_(timeout), proc_(proc), handler_(handler), groupId_(std::move(groupId))
        {
        }

        MessageCollector(const MessageCollector &) = delete;
        MessageCollector &operator=(const MessageCollector &) = delete;

        void addData(TgBot::Message::Ptr msg)
        {
            messages_.push_back(std::move(msg));
            timer_.expires_after(timeout_);

            if (!running_)
            {
                running_ = true;
                asio::co_spawn(ex_, run(shared_from_this()), asio::detached);
            }
        }

    private:
        asio::any_io_executor            ex_;
        asio::steady_timer               timer_;
        std::chrono::milliseconds        timeout_;
        std::vector<TgBot::Message::Ptr> messages_;
        bool                             running_ = false;

        MessagesProcessor *proc_;
        MessagesHandler    handler_;
        std::string        groupId_;

    private:
        static asio::awaitable<void> run(Ptr self)
        {
            for (;;)
            {
                boost::system::error_code ec;
                co_await self->timer_.async_wait(asio::redirect_error(asio::use_awaitable, ec));

                if (ec == asio::error::operation_aborted)
                    continue; // таймер перезапущен новым сообщением

                if (ec)
                    std::cerr << "MessageCollector timer error: " << ec.message() << std::endl;

                break; // таймаут истёк (или фатальная ошибка) — собираем группу
            }

            // Сначала убираем себя из map: новые сообщения этой группы
            // создадут свежий коллектор, а хендлеру map больше не нужен.
            self->proc_->collections_.erase(self->groupId_);

            if (!self->messages_.empty())
                co_await (self->proc_->*self->handler_)(std::move(self->messages_));
            // self уничтожится здесь, после полного завершения корутины.
        }
    };

    struct ImagePart
    {
        std::string iamegId;
        std::string base64;
    };

public:
    MessagesProcessor(transport::TgBotMessageSender &sender, core::Operator &op, store::ImageStore &store)
        : sender_(sender), operator_(op), store_(store)
    {
    }

    asio::awaitable<void> addMessage(TgBot::Message::Ptr msg)
    {
        if (msg->mediaGroupId)
        {
            co_await collect(std::move(msg), &MessagesProcessor::addMessages);
            co_return;
        }

        if (!msg->text)
            co_return;

        auto info = std::make_shared<core::OperationInfo>(msg->chat->id);
        co_await operator_.addMessage(info, msg->text.value());
    }

    asio::awaitable<void> processMessage(TgBot::Message::Ptr msg)
    {
        if (msg->mediaGroupId)
        {
            co_await collect(std::move(msg), &MessagesProcessor::processMessages);
            co_return;
        }
        std::string            text;
        std::vector<ImagePart> images;
        openai::ChatIdType     chatId = 0;

        co_await appendSendDataFromMessage(text, images, chatId, msg);
        if (!chatId)
            co_return;
        auto info = std::make_shared<core::OperationInfo>(msg->chat->id);
        // Все картинки заполняем и отправляем отдельно
        for (auto &&[id, b64] : images)
        {
            openai::AdditionalsToMessage prAdd;
            openai::HistoryUtils::addPhotoToAdditionals(prAdd, std::move(b64));
            co_await operator_.addMessage(info, utils::Format::format("Техническое сообщение: следующее изображение имеет image_id = {}", id), std::move(prAdd));
        }
        co_await operator_.processMessage(info, std::move(text));
    }

    asio::awaitable<void> addMessages(std::vector<TgBot::Message::Ptr> msgs)
    {
        std::string            text;
        std::vector<ImagePart> images;
        openai::ChatIdType     chatId = 0;
        for (const auto &msg : msgs)
            co_await appendSendDataFromMessage(text, images, chatId, msg);
        if (!chatId)
            co_return;
        auto info = std::make_shared<core::OperationInfo>(chatId);
        // Все картинки заполняем и отправляем отдельно
        for (auto &&[id, b64] : images)
        {
            openai::AdditionalsToMessage prAdd;
            openai::HistoryUtils::addPhotoToAdditionals(prAdd, std::move(b64));
            co_await operator_.addMessage(info, utils::Format::format("Техническое сообщение: следующее изображение имеет image_id = {}", id), std::move(prAdd));
        }
        co_await operator_.addMessage(info, std::move(text));
    }

    asio::awaitable<void> processMessages(std::vector<TgBot::Message::Ptr> msgs)
    {
        std::string            text;
        std::vector<ImagePart> images;
        openai::ChatIdType     chatId = 0;
        for (const auto &msg : msgs)
            co_await appendSendDataFromMessage(text, images, chatId, msg);
        if (!chatId)
            co_return;
        auto info = std::make_shared<core::OperationInfo>(chatId);
        // Все картинки заполняем и отправляем отдельно
        for (auto &&[id, b64] : images)
        {
            openai::AdditionalsToMessage prAdd;
            openai::HistoryUtils::addPhotoToAdditionals(prAdd, std::move(b64));
            co_await operator_.addMessage(info, utils::Format::format("Техническое сообщение: следующее изображение имеет image_id = {}", id), std::move(prAdd));
        }

        co_await operator_.processMessage(info, std::move(text));
    }

private:
    transport::TgBotMessageSender &sender_;
    core::Operator                &operator_;
    store::ImageStore             &store_;

    std::unordered_map<std::string, MessageCollector::Ptr> collections_;

private:
    asio::awaitable<void> processImages(openai::AdditionalsToMessage &adds)
    {

        co_return;
    }

    asio::awaitable<std::optional<ImagePart>> addPhoto(std::vector<TgBot::PhotoSize::Ptr> &photos)
    {
        std::string      fileId = photos.at(0)->fileId;
        TgBot::File::Ptr file = co_await sender_.getFile(std::move(fileId));
        if (!file || !file->filePath)
            co_return std::nullopt;

        std::string binFile = co_await sender_.downloadFile(std::move(file->filePath.value()));

        auto base64Pr = utils::Base64::encode(binFile);
        if (!base64Pr)
            co_return std::nullopt;

        ImagePart res;
        res.base64 = base64Pr.value();
        res.iamegId = store_.saveImage(std::move(binFile));

        co_return res;
    }

    asio::awaitable<void> appendSendDataFromMessage(std::string &text, std::vector<ImagePart> &imagesParts, openai::ChatIdType &chatId, TgBot::Message::Ptr msg)
    {
        if (msg->text)
            text = std::move(msg->text.value());
        if (msg->caption)
            text = std::move(msg->caption.value());
        if (msg->chat)
            chatId = msg->chat->id;

        if (msg->photo && !msg->photo->empty())
        {
            auto imageId = co_await addPhoto(msg->photo.value());
            if (imageId)
                imagesParts.push_back(std::move(imageId.value()));
        }
    }

    asio::awaitable<void> collect(TgBot::Message::Ptr msg, MessagesHandler handler)
    {
        const std::string &id = msg->mediaGroupId.value();

        auto it = collections_.find(id);
        if (it == collections_.end())
        {
            auto ex = co_await asio::this_coro::executor;
            auto col = std::make_shared<MessageCollector>(std::move(ex), std::chrono::milliseconds(500), this, handler, id);
            it = collections_.emplace(id, std::move(col)).first;
        }
        it->second->addData(std::move(msg));
    }
};

} // namespace handlers