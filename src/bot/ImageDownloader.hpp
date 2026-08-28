#pragma once

#include "Error.hpp"
#include <tgbot/Api.h>
#include <utils/Base64.hpp>
#include <utils/Types.hpp>

namespace bot
{
class ImageDownloader
{
public:
    ImageDownloader(const TgBot::Api &api, const std::string &token)
        : api_(api),
          token_(token)
    {
    }

    utils::SyncResult<void> downloadImage(std::string &img, const std::string &fileId)
    {
        std::shared_ptr<TgBot::File> fileInfo = api_.getFile(fileId);
        if (fileInfo->filePath)
            return std::unexpected(Error::FileNotDownloaded);

        std::string filePath = "https://api.telegram.org/file/bot" + token_ + '/' + *fileInfo->filePath;

        std::string binImg = api_.downloadFile(filePath);
        auto        encodeRes = utils::Base64::encode(binImg);
        if (!encodeRes)
            return std::unexpected(encodeRes.error());

        img = std::move(encodeRes.value());
        return utils::empty;
    }

    utils::AsyncResult<void> downloadImageAsync(std::string &img, const std::string &fileId)
    {
        co_return downloadImage(img, fileId);
    }

private:
    const TgBot::Api  &api_;
    const std::string &token_;
};
} // namespace bot