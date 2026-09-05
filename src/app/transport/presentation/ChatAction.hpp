#pragma once

#include <cstdint>

namespace transport
{
enum class ChatAction : std::uint8_t
{
    typing,
    upload_photo,
    record_video,
    upload_video,
    record_voice,
    upload_voice,
    upload_document,
    choose_sticker,
    find_location,
    record_video_note,
    upload_video_note
};
}