#pragma once

#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "Role.hpp"
#include "Tools.hpp"

namespace dto
{

// ============================================================================
//  Части content
// ============================================================================

struct TextPart
{
    std::string type = "text";
    std::string text;
};

struct ImageUrl
{
    std::string url;

    /// "auto" | "low" | "high".
    /// low — фиксированные 85 токенов, high — на порядок дороже.
    std::optional<std::string> detail;
};

struct ImagePart
{
    std::string type = "image_url";
    ImageUrl    image_url;
};

struct InputAudio
{
    std::string data;   ///< base64, ссылок для аудио не предусмотрено
    std::string format; ///< "wav" | "mp3"
};

struct AudioPart
{
    std::string type = "input_audio";
    InputAudio  input_audio;
};

struct FileData
{
    std::optional<std::string> file_id;
    std::optional<std::string> filename;
    std::optional<std::string> file_data; ///< base64
};

struct FilePart
{
    std::string type = "file";
    FileData    file;
};

// Отказ
struct RefusalPart
{
    std::string type = "refusal";
    std::string refusal;
};

using ContentPart = std::variant<TextPart, ImagePart, AudioPart, FilePart, RefusalPart>;
using Content = std::variant<std::string, std::vector<ContentPart>>;

// ============================================================================
//  Сообщение запроса
// ============================================================================
struct Message
{
    Role                                 role;
    std::optional<Content>               content;      // nullable, если tool_calls непусто, а текста нет — null
    std::optional<std::string>           name;         // Все роли
    std::optional<std::string>           tool_call_id; // Только role == tool. Должен точно совпадать с id из tool_calls предыдущего
    std::optional<std::vector<ToolCall>> tool_calls;   // Только role == assistant.
    std::optional<std::string>           refusal;      // Только role == assistant, если модель отказалась отвечать по причине refusal
};

} // namespace dto
