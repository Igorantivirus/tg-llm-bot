#pragma once

#include <random>
#include <string>
#include <string_view>

namespace utils
{

/// @brief Сборщик тела multipart/form-data.
///
/// Формат простой: части разделены строкой "--<boundary>", каждая несёт свои
/// заголовки, пустую строку и значение; тело закрывает "--<boundary>--".
/// Бинарные данные кладутся сырыми — в отличие от JSON, base64 не нужен.
///
/// Части накапливаются в одной строке, поэтому для больших картинок стоит
/// заранее вызвать reserve(): иначе каждая реаллокация копирует всё тело.
class MultipartBuilder
{
public:
    MultipartBuilder()
        : boundary_(generateBoundary())
    {
    }

    /// @brief Заголовок Content-Type с объявленным boundary.
    std::string contentType() const
    {
        return "multipart/form-data; boundary=" + boundary_;
    }

    void reserve(const std::size_t size)
    {
        body_.reserve(size);
    }

    /// @brief Текстовое поле формы.
    void addField(const std::string_view name, const std::string_view value)
    {
        openPart(name);
        body_ += "\r\n\r\n";
        body_ += value;
        body_ += "\r\n";
    }

    /// @brief Файловое поле: отличается от текстового наличием filename и своего Content-Type.
    void addFile(const std::string_view name, const std::string_view fileName, const std::string_view contentType, const std::string_view content)
    {
        openPart(name);
        body_ += "; filename=\"";
        body_ += fileName;
        body_ += "\"\r\nContent-Type: ";
        body_ += contentType;
        body_ += "\r\n\r\n";
        body_ += content;
        body_ += "\r\n";
    }

    /// @brief Дописывает закрывающий разделитель и отдаёт тело перемещением.
    /// После вызова сборщик использовать повторно нельзя.
    std::string finish()
    {
        body_ += "--";
        body_ += boundary_;
        body_ += "--\r\n";
        return std::move(body_);
    }

private:
    std::string boundary_;
    std::string body_;

private:
    void openPart(const std::string_view name)
    {
        body_ += "--";
        body_ += boundary_;
        body_ += "\r\nContent-Disposition: form-data; name=\"";
        body_ += name;
        body_ += '"';
    }

    /// @brief Разделитель не должен встретиться внутри данных, поэтому он случайный и длинный.
    static std::string generateBoundary()
    {
        static constexpr std::string_view alphabet = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        static constexpr std::size_t      randomPartSize = 24;

        std::random_device                                 rd;
        std::mt19937                                       gen(rd());
        std::uniform_int_distribution<std::size_t> dist(0, alphabet.size() - 1);

        std::string boundary = "----TgLlmBotBoundary";
        boundary.reserve(boundary.size() + randomPartSize);
        for (std::size_t i = 0; i < randomPartSize; ++i)
            boundary += alphabet[dist(gen)];
        return boundary;
    }
};

} // namespace utils
