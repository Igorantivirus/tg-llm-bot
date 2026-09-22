#pragma once

#include <ios>
#include <iostream>
#include <fstream>
#include <net/HttpClient.hpp>
#include <utils/MultipartBuilder.hpp>
#include <utils/Parser.hpp>
#include <utils/Types.hpp>

#include <openai/Error.hpp>
#include <openai/api/ApiResponseGenerator.hpp>
#include <openai/dto/ChatCompletions/Request.hpp>
#include <openai/dto/ChatCompletions/Response.hpp>
#include <openai/dto/Image/EditImageRequest.hpp>
#include <openai/dto/Image/GenerateImageRequest.hpp>
#include <openai/dto/Image/ImageResponse.hpp>
#include <openai/dto/ModelsResponse.hpp>

namespace openai
{

class Api
{
public:
    Api(asio::any_io_executor ex, const std::size_t tcpConnsCount, std::string host, std::string port, std::string token)
        : http_(ex, tcpConnsCount),
          fullHost_(isNum(port) ? host + ':' + port : host),
          host_(std::move(host)),
          port_(std::move(port)),
          authorization_("Bearer " + token),
          token_(std::move(token))
    {
    }

    utils::AsyncResult<dto::ModelsResponse> models()
    {
        net::BeastRequest req(http::verb::get, "/v1/models", 11);
        initRequestFields(req);

        auto res = co_await http_.request(host_, port_, std::move(req));
        if (!res)
            co_return std::unexpected(res.error());
        auto resp = std::move(res.value());
        co_return utils::deserialize<dto::ModelsResponse>(resp.isStreaming() ? co_await resp.streamBody().readAll() : resp.stringBody());
    }

    utils::AsyncResult<dto::ImageResponse> imagesGeneration(dto::GenerateImageRequest dto)
    {
        co_return co_await imagesRequest("/v1/images/generations", std::move(dto));
    }

    /// @brief Редактирование: эндпоинт принимает только multipart/form-data,
    /// поэтому тело собирается вручную, а не сериализуется из dto.
    utils::AsyncResult<dto::ImageResponse> imagesEdit(dto::EditImageRequest dto)
    {
        if (dto.images.empty())
            co_return std::unexpected(Error::EmptyImagesToEdit);

        utils::MultipartBuilder mp;
        mp.reserve(multipartBodySize(dto));

        mp.addField("prompt", dto.prompt);
        // Несколько картинок OpenAI ждёт под именем "image[]", одну — под "image".
        const std::string_view imageField = dto.images.size() > 1 ? "image[]" : "image";
        for (const auto &image : dto.images)
            addImagePart(mp, imageField, "image", image);
        if (dto.mask)
            addImagePart(mp, "mask", "mask", dto.mask.value());


        addOptionalField(mp, "model", dto.model);
        addOptionalField(mp, "size", dto.size);
        addOptionalField(mp, "user", dto.user);
        if (dto.n)
            mp.addField("n", std::to_string(dto.n.value()));
        if (auto field = enumField(dto.quality); field)
            mp.addField("quality", field.value());
        if (auto field = enumField(dto.background); field)
            mp.addField("background", field.value());
        if (auto field = enumField(dto.output_format); field)
            mp.addField("output_format", field.value());

        net::BeastRequest req(http::verb::post, "/v1/images/edits", 11);
        req.body() = mp.finish();
        initRequestFields(req, mp.contentType());

        co_return co_await sendImagesRequest(std::move(req));
    }

    utils::AsyncResult<ApiResponseGenerator> chatCompletions(dto::ChatCompletionsRequest dto)
    {
        {
            std::ofstream out("log.log", std::ios_base::app);
            out << "Request:\n";
            auto j = utils::serialize(dto);
            if (j)
                out << j.value() << '\n';
            else
                out << j.error() << '\n';
        }

        net::BeastRequest req(http::verb::post, "/v1/chat/completions", 11);
        if (auto sdto = utils::serialize(dto); sdto)
            req.body() = sdto.value();
        else
            co_return std::unexpected(sdto.error());
        initRequestFields(req);

        auto res = co_await http_.request(host_, port_, std::move(req));
        if (!res)
            co_return std::unexpected(res.error());
        auto resp = std::move(res.value());
        if (resp.header.result_int() / 100 != 2)
        {
            std::cout << resp.stringBody() << '\n';
            std::cout << "Error: " << resp.header.result_int() << " code\n";
            co_return std::unexpected(Error::FromServer);
        }

        if (!resp.isStreaming()) // Тело сразу есть
        {
            auto resDto = utils::deserialize<dto::ChatCompletionsResponse>(resp.stringBody());
            if (!resDto)
                co_return std::unexpected(resDto.error());
            co_return ApiResponseGenerator(std::move(resDto.value()));
        }
        co_return ApiResponseGenerator{std::move(resp.streamBody())};
    }

private:
    net::HttpClient http_;

    const std::string fullHost_;
    const std::string host_;
    const std::string port_;
    const std::string authorization_;
    const std::string token_;

private:
    constexpr static bool isNum(const std::string &s)
    {
        int res;
        auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), res);
        return ec == std::errc{} && ptr == s.data() + s.size();
    }

private:
    /// @brief JSON-вариант images-запроса (генерация).
    template <typename Dto>
    utils::AsyncResult<dto::ImageResponse> imagesRequest(const std::string_view path, Dto dto)
    {
        net::BeastRequest req(http::verb::post, path, 11);
        if (auto sdto = utils::serialize(dto); sdto)
            req.body() = sdto.value();
        else
            co_return std::unexpected(sdto.error());
        initRequestFields(req);

        co_return co_await sendImagesRequest(std::move(req));
    }

    /// @brief Отправка и разбор ответа, общие для обоих images-эндпоинтов.
    utils::AsyncResult<dto::ImageResponse> sendImagesRequest(net::BeastRequest req)
    {
        auto res = co_await http_.request(host_, port_, std::move(req));
        if (!res)
            co_return std::unexpected(res.error());
        auto resp = std::move(res.value());
        if (resp.header.result_int() / 100 != 2)
        {
            std::cout << "Images request error " << resp.header.result_int() << ": " << resp.stringBody() << '\n';
            co_return std::unexpected(Error::FromServer);
        }

        if (resp.isStreaming())
            co_return std::unexpected(Error::UnsoportedStream);
        co_return utils::deserialize<dto::ImageResponse>(resp.stringBody());
    }

    static void addImagePart(utils::MultipartBuilder &mp, const std::string_view partName, const std::string_view baseName, const dto::ImageFile &image)
    {
        mp.addFile(partName, std::string(baseName) + '.' + std::string(image.extension()), image.mimeType(), image.data);
    }

    static void addOptionalField(utils::MultipartBuilder &mp, const std::string_view name, const std::optional<std::string> &value)
    {
        if (value)
            mp.addField(name, value.value());
    }

    /// @brief Имя enum-значения так, как его ждёт сервер (с учётом переименований Jsonser).
    template <typename E>
    static std::optional<std::string> enumField(const std::optional<E> &value)
    {
        if (!value)
            return std::nullopt;
        const auto index = magic_enum::enum_index(value.value());
        if (!index)
            return std::nullopt;
        static constexpr auto names = jsonser::getEnumNames<E>();
        if (!names[*index])
            return std::nullopt;
        return std::string(names[*index].value());
    }

    /// @brief Оценка размера тела: картинки плюс запас на заголовки частей.
    static std::size_t multipartBodySize(const dto::EditImageRequest &dto)
    {
        static constexpr std::size_t partOverhead = 256;

        std::size_t size = dto.prompt.size() + partOverhead * 12;
        for (const auto &image : dto.images)
            size += image.data.size() + partOverhead;
        if (dto.mask)
            size += dto.mask.value().data.size() + partOverhead;
        return size;
    }

private:
    void initRequestFields(net::BeastRequest &req, const std::string_view contentType = "application/json") const
    {
        req.set(http::field::host, fullHost_);
        req.set(http::field::content_type, contentType);
        req.set(http::field::authorization, authorization_);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.prepare_payload();
    }
};

} // namespace openai