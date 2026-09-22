#pragma once

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <tgbot/Types.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <app/core/Operator.hpp>
#include <app/store/ImageEntry.hpp>
#include <app/store/ImageStore.hpp>
#include <app/tika/UnpackerAll.hpp>
#include <app/transport/TgBotMessageSender.hpp>
#include <openai/dto/ChatCompletions/Message.hpp>
#include <utils/Base64.hpp>

#include "MessageBuildInfo.hpp"

namespace handlers
{

class MessagesProcessor
{
private:
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

public:
    MessagesProcessor(transport::TgBotMessageSender &sender, core::Operator &op, store::ImageStore &store, tika::UnpackerAll &unpacker)
        : sender_(sender), operator_(op), store_(store), unpacker_(unpacker)
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
    tika::UnpackerAll             &unpacker_;

    std::unordered_map<std::string, MessageCollector::Ptr> collections_;

private:
    inline dto::ImageFormat mimeTypeToFormat(std::string_view mimeType)
    {
        static const std::unordered_map<std::string_view, dto::ImageFormat> s_map = {
            {"image/png",  dto::ImageFormat::png },
            {"image/jpeg", dto::ImageFormat::jpeg},
            {"image/jpg",  dto::ImageFormat::jpeg},
            {"image/webp", dto::ImageFormat::webp},
            {"image/gif",  dto::ImageFormat::gif },
            {"image/tiff", dto::ImageFormat::tiff},
            {"image/tif",  dto::ImageFormat::tiff},
            {"image/bmp",  dto::ImageFormat::bmp },
        };

        if (auto it = s_map.find(mimeType); it != s_map.end())
            return it->second;
        return dto::ImageFormat::jpeg; // fallback
    }

    asio::awaitable<void> processImages(openai::AdditionalsToMessage &adds)
    {

        co_return;
    }

    asio::awaitable<void> addPhoto(std::vector<TgBot::PhotoSize::Ptr> &photos, MessageBuildInfo &info)
    {
        if (photos.empty())
            co_return;

        const TgBot::PhotoSize::Ptr &photo = photos.back();

        TgBot::File::Ptr file = co_await sender_.getFile(photo->fileId);
        if (!file || !file->filePath)
            co_return;

        std::string binFile = co_await sender_.downloadFile(std::move(file->filePath.value()));

        auto base64Pr = utils::Base64::encode(binFile);
        if (!base64Pr)
            co_return;

        // Размеры Telegram сообщает сам, разбирать заголовок картинки не нужно.
        store::ImageEntry entry;
        entry.format = dto::ImageFormat::jpeg; // photo всегда пережато в jpeg
        entry.size = store::ImageEntry::makeSize(photo->width, photo->height);
        entry.data = std::move(binFile);

        ContentPart res;
        res.data = std::move(base64Pr.value());
        res.id = store_.saveImage(std::move(entry));
        info.images.push_back(res);
    }
    asio::awaitable<void> addFile(TgBot::Document &doc, MessageBuildInfo &info)
    {
        TgBot::File::Ptr file = co_await sender_.getFile(doc.fileId);
        if (!file || !file->filePath)
            co_return;

        std::string binFile = co_await sender_.downloadFile(std::move(file->filePath.value()));

        auto unpackRes = co_await unpacker_.unpackAll(std::move(binFile), doc.fileName.value_or("unknown-file-name"));
        if (!unpackRes)
        {
            std::cout << "Error of unpack: " << unpackRes.error().message() << '\n';
            co_return;
        }

        for (auto &&img : unpackRes->imageFiles)
        {
            auto base64Pr = utils::Base64::encode(binFile);
            if (!base64Pr)
                continue;
            store::ImageEntry entry;
            entry.format = mimeTypeToFormat(img.type);
            entry.size = store::ImageEntry::makeSize(img.width, img.height);
            entry.data = std::move(binFile);

            ContentPart res;
            res.data = std::move(base64Pr.value());
            res.id = store_.saveImage(std::move(entry));
            info.images.push_back(std::move(res));
        }
        for (auto &&file : unpackRes->textParsedFiles)
        {
            ContentPart filePart;
            filePart.data = std::move(file.data);
            filePart.id = std::move(file.fileName);
            info.files.push_back(std::move(filePart));
        }
        co_return;
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
            co_await addPhoto(msg->photo.value(), info);
        if (msg->document)
            co_await addFile(*msg->document.get(), info);
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