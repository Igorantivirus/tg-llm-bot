#pragma once

#include <dto/ChatCompletions/Message.hpp>

namespace openai
{

struct AdditionalsToMessage
{
    std::vector<dto::ImageUrl>   imagesB64;
    std::vector<dto::InputAudio> audiosB64;
    std::vector<dto::FileData>   filesB64;

    bool empty() const
    {
        return imagesB64.empty() && audiosB64.empty() && filesB64.empty();
    }
    std::size_t sumOfDataParts() const
    {
        return imagesB64.size() + audiosB64.size() + filesB64.size();
    }
};
} // namespace openai