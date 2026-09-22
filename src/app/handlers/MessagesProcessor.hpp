#pragma once

#include "app/store/ImageEntry.hpp"
#include "app/transport/TgBotMessageSender.hpp"
#include "openai/chatssettings/AdditionalsToMessage.hpp"
#include "openai/chatssettings/HistoryUtils.hpp"
#include "openai/chatssettings/Types.hpp"
#include "openai/dto/ChatCompletions/Message.hpp"
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

    struct ContentPart
    {
        std::string id;
        std::string base64;
    };

    struct MessageBuildInfo
    {
        openai::ChatIdType       chat;
        std::string              message;
        std::vector<ContentPart> images;

        static dto::Content toMessage(MessageBuildInfo info)
        {
            if (info.images.empty())
                return std::move(info.message);
            std::vector<dto::ContentPart> res;
            res.push_back(getTextPart(std::move(info.message)));

            for (auto &&[id, b64] : info.images)
            {
                res.push_back(getTextPart("id следующего изображения = " + id));
                res.push_back(getJpegPart(std::move(b64)));
            }

            return res;
        }

    private:
        static dto::TextPart getTextPart(std::string msg)
        {
            dto::TextPart part;
            part.text = std::move(msg);
            return part;
        }
        static dto::ImagePart getJpegPart(std::string b64)
        {
            b64.insert(0, openai::HistoryUtils::getBase64JpegPrefix());

            dto::ImageUrl url;
            url.url = std::move(b64);

            dto::ImagePart part;
            part.image_url = std::move(url);
            return part;
        }
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

        MessageBuildInfo msgInfo;
        co_await appendSendDataFromMessage(msgInfo, msg);
        if (!msgInfo.chat)
            co_return;

        auto info = std::make_shared<core::OperationInfo>(msgInfo.chat);

        dto::Content dto = MessageBuildInfo::toMessage(std::move(msgInfo));
        co_await operator_.addMessage(info, std::move(dto));
    }

    asio::awaitable<void> processMessage(TgBot::Message::Ptr msg)
    {
        if (msg->mediaGroupId)
        {
            co_await collect(std::move(msg), &MessagesProcessor::processMessages);
            co_return;
        }
        MessageBuildInfo msgInfo;
        co_await appendSendDataFromMessage(msgInfo, msg);
        if (!msgInfo.chat)
            co_return;

        auto info = std::make_shared<core::OperationInfo>(msgInfo.chat);

        dto::Content dto = MessageBuildInfo::toMessage(std::move(msgInfo));
        co_await operator_.processMessage(info, std::move(dto));
    }

    asio::awaitable<void> addMessages(std::vector<TgBot::Message::Ptr> msgs)
    {
        MessageBuildInfo msgInfo;

        for (const auto &msg : msgs)
            co_await appendSendDataFromMessage(msgInfo, msg);
        if (!msgInfo.chat)
            co_return;

        auto info = std::make_shared<core::OperationInfo>(msgInfo.chat);

        dto::Content dto = MessageBuildInfo::toMessage(std::move(msgInfo));
        co_await operator_.addMessage(info, std::move(dto));
    }

    asio::awaitable<void> processMessages(std::vector<TgBot::Message::Ptr> msgs)
    {
        MessageBuildInfo msgInfo;

        for (const auto &msg : msgs)
            co_await appendSendDataFromMessage(msgInfo, msg);
        if (!msgInfo.chat)
            co_return;

        auto info = std::make_shared<core::OperationInfo>(msgInfo.chat);

        dto::Content dto = MessageBuildInfo::toMessage(std::move(msgInfo));
        co_await operator_.processMessage(info, std::move(dto));
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

    asio::awaitable<std::optional<ContentPart>> addPhoto(std::vector<TgBot::PhotoSize::Ptr> &photos)
    {
        if (photos.empty())
            co_return std::nullopt;

        // Telegram присылает варианты одного фото по возрастанию разрешения:
        // последний — самый крупный из доступных. Оригинал приходит только
        // документом, фото всегда пережато.
        const TgBot::PhotoSize::Ptr &photo = photos.back();

        TgBot::File::Ptr file = co_await sender_.getFile(photo->fileId);
        if (!file || !file->filePath)
            co_return std::nullopt;

        std::string binFile = co_await sender_.downloadFile(std::move(file->filePath.value()));

        auto base64Pr = utils::Base64::encode(binFile);
        if (!base64Pr)
            co_return std::nullopt;

        // Размеры Telegram сообщает сам, разбирать заголовок картинки не нужно.
        store::ImageEntry entry;
        entry.format = dto::ImageOutputFormat::jpeg; // photo всегда пережато в jpeg
        entry.size = store::ImageEntry::makeSize(photo->width, photo->height);
        entry.data = std::move(binFile);

        ContentPart res;
        res.base64 = base64Pr.value();
        res.id = store_.saveImage(std::move(entry));

        co_return res;
    }

    asio::awaitable<void> appendSendDataFromMessage(MessageBuildInfo &info, TgBot::Message::Ptr msg)
    {
        if (msg->text)
            info.message = std::move(msg->text.value());
        if (msg->caption)
            info.message = std::move(msg->caption.value());
        if (msg->chat)
            info.chat = msg->chat->id;

        if (msg->photo && !msg->photo->empty())
        {
            auto imageId = co_await addPhoto(msg->photo.value());
            if (imageId)
                info.images.push_back(std::move(imageId.value()));
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