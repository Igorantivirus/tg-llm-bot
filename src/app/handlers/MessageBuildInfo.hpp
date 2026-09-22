#pragma once

#include <openai/chatssettings/HistoryUtils.hpp>
#include <openai/chatssettings/Types.hpp>
#include <openai/dto/ChatCompletions/Message.hpp>

namespace handlers
{
struct ContentPart
{
    std::string id;
    std::string data;
};
struct MessageBuildInfo
{
public:
    openai::ChatIdType       chat;
    std::string              message;
    std::vector<ContentPart> images;
    std::vector<ContentPart> files;

public:
    static dto::Content toMessage(MessageBuildInfo info)
    {
        if (info.images.empty() && info.files.empty())
            return std::move(info.message);
        std::vector<dto::ContentPart> res;
        res.push_back(getTextPart(std::move(info.message)));

        for (auto &&[id, b64] : info.images)
        {
            res.push_back(getTextPart("next image id = " + id));
            res.push_back(getJpegPart(std::move(b64)));
        }
        for (auto &&[id, text] : info.files)
        {
            res.push_back(getTextPart("next file with name = " + id));
            res.push_back(getTextPart(std::move(text)));
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
} // namespace handlers