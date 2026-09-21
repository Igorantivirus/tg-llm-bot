#pragma once

#include "openai/chatssettings/HistoryUtils.hpp"
#include "openai/dto/ChatCompletions/Message.hpp"
#include <app/store/ImageStore.hpp>
#include <memory>
#include <openai/Tools/Tool.hpp>
#include <openai/api/Api.hpp>
#include <openai/dto/Image/EditImageRequest.hpp>
#include <openai/dto/Image/ImageResponse.hpp>
#include <utils/Base64.hpp>
#include <utils/NonNullCopybleUniquePtr.hpp>
#include <utils/Parser.hpp>

namespace tools
{
class CreateImageToolResult : public openai::ToolResult
{
public:
    static inline const std::string calledFunctionName = "create_image";

    struct ImageMeta
    {
        bool                       success = true;
        std::vector<std::string>   imageIds;
        std::optional<std::string> size;
    };

public:
    CreateImageToolResult(dto::ImageResponse dto, store::ImageStore &imgStore)
        : ToolResult(calledFunctionName), dto_(std::move(dto)), store_(&imgStore)
    {
        meta_.success = dto_.data.has_value();
        if (!meta_.success)
            return;

        for (auto &&img : dto_.data.value())
        {
            if (!img.b64_json)
                continue;
            auto binImagePr = utils::Base64::decode(img.b64_json.value());
            if (!binImagePr)
                continue;
            meta_.imageIds.push_back(imgStore.saveImage(std::move(binImagePr.value())));
        }
        meta_.size = dto_.size;
    }

    std::string toString() const override
    {
        auto resp = utils::serialize(meta_);
        if (!resp)
            return resp.error().message();
        return resp.value();
    }

    bool needToSendAdditionalMessage() const override
    {
        return true;
    }

    std::vector<dto::ContentPart> getAdditionalMessage() const override
    {
        std::vector<dto::ContentPart> part;
        for (const auto &id : meta_.imageIds)
        {
            const std::string &imageBin = store_->getImageById(id);
            auto               base64Pr = utils::Base64::encode(imageBin);
            if (!base64Pr)
                continue;

            dto::TextPart text;
            text.text = "Result of create_image: image with id = " + id;
            dto::ImagePart imgPart;
            imgPart.image_url = dto::ImageUrl{.url = openai::HistoryUtils::getBase64JpegPrefix() + base64Pr.value()};
            part.push_back(std::move(text));
            part.push_back(std::move(imgPart));
        }
        return part;
    }

    const dto::ImageResponse &getDto() const
    {
        return dto_;
    }

private:
    ImageMeta          meta_;
    dto::ImageResponse dto_;
    store::ImageStore *store_;
};

class CreateImage : public openai::Tool
{
public:
    // TODO: вынести в конфиг вместе с выбором модели
    static inline const std::string generationModel = "qwen-edit-nsfw";
    static inline const std::string editionModel = "qwen-edit-nsfw";

    enum class ActionType
    {
        generate,
        edit
    };
    enum class AspectRatioType
    {
        square,
        portrait,
        landscape
    };
    struct Params
    {
        std::string              prompt;
        ActionType               action = ActionType::generate;
        AspectRatioType          aspect_ratio = AspectRatioType::square;
        unsigned short           image_count = 1;
        /// Непустой список означает редактирование: эти картинки берутся из store
        /// и уходят в модель редактирования вместе с промтом.
        std::vector<std::string> image_ids;
    };

public:
    CreateImage(openai::Api &api, store::ImageStore &imgStore)
        : api_(api), imgStore_(imgStore)
    {
    }

    utils::AsyncResult<openai::ToolResult::Ptr> run(std::string args) override
    {
        auto dto = utils::deserialize<Params>(args);
        if (!dto)
            co_return std::unexpected(dto.error());
        Params params = std::move(dto.value());

        // Редактирование запрашивается наличием картинок на входе, а не полем action:
        // модель заполняет action не всегда, а image_ids без редактирования бессмысленны.
        auto res = params.image_ids.empty()
                       ? co_await generate(params)
                       : co_await edit(params);
        if (!res)
        {
            std::cout << "Create message error: " << res.error() << '\n';
            co_return std::unexpected(res.error());
        }

        co_return std::make_shared<CreateImageToolResult>(std::move(res.value()), imgStore_);
    }
    std::string name() const override
    {
        return CreateImageToolResult::calledFunctionName;
    }
    std::string description() const override
    {
        return "Generate a new image or edit the most recent image in the conversation.";
    }
    std::optional<dto::schema::Object> parameters() const override
    {
        dto::schema::Object obj;
        obj.type = "object";

        // Create properties
        dto::schema::Properties properties;

        // Action property
        dto::schema::Schema action_schema;
        dto::schema::String action_str;
        action_str.type = "string";
        action_str.enum_field = std::vector<std::string>{"generate", "edit"};
        action_str.description = "Use 'edit' to modify existing images (also fill image_ids), 'generate' for a new one.";
        action_schema.value = std::move(action_str);
        properties["action"] = utils::NonNullCopybleUniquePtr<dto::schema::Schema>(std::move(action_schema));

        // Prompt property
        dto::schema::Schema prompt_schema;
        dto::schema::String prompt_str;
        prompt_str.type = "string";
        prompt_str.description = "Detailed English description of the desired image. For edits, describe the change, not the whole scene. Expand on the user's request with concrete visual detail: subject, setting, lighting, composition.";
        prompt_schema.value = std::move(prompt_str);
        properties["prompt"] = utils::NonNullCopybleUniquePtr<dto::schema::Schema>(std::move(prompt_schema));

        // Aspect ratio property
        dto::schema::Schema aspect_schema;
        dto::schema::String aspect_str;
        aspect_str.type = "string";
        aspect_str.enum_field = std::vector<std::string>{"square", "portrait", "landscape"};
        aspect_str.description = "Choose based on intended use. Default to square when unclear.";
        aspect_schema.value = std::move(aspect_str);
        properties["aspect_ratio"] = utils::NonNullCopybleUniquePtr<dto::schema::Schema>(std::move(aspect_schema));

        // Image count property (1 to 9 inclusive)
        dto::schema::Schema  image_count_schema;
        dto::schema::Integer image_count_int;
        image_count_int.type = "integer";
        image_count_int.minimum = 1;
        image_count_int.maximum = 9;
        image_count_int.description = "Number of images to generate. Must be between 1 and 9 inclusive.";
        image_count_schema.value = std::move(image_count_int);
        properties["image_count"] = utils::NonNullCopybleUniquePtr<dto::schema::Schema>(std::move(image_count_schema));

        // Image ids property (редактирование)
        dto::schema::Schema image_ids_schema;
        dto::schema::Array  image_ids_arr;
        image_ids_arr.type = "array";
        image_ids_arr.description = "Ids of the images to edit, taken from previous create_image results. Leave empty to generate a new image; fill it only when modifying images that already exist in this conversation.";
        dto::schema::Schema image_id_schema;
        dto::schema::String image_id_str;
        image_id_str.type = "string";
        image_id_schema.value = std::move(image_id_str);
        image_ids_arr.items = utils::NonNullCopybleUniquePtr<dto::schema::Schema>(std::move(image_id_schema));
        image_ids_schema.value = std::move(image_ids_arr);
        properties["image_ids"] = utils::NonNullCopybleUniquePtr<dto::schema::Schema>(std::move(image_ids_schema));

        obj.properties = std::move(properties);

        // Required fields
        obj.required = std::vector<std::string>{"prompt"};

        return obj;
    }

private:
    openai::Api       &api_;
    store::ImageStore &imgStore_;

private:
    utils::AsyncResult<dto::ImageResponse> generate(Params &params)
    {
        dto::GenerateImageRequest req;
        req.prompt = std::move(params.prompt);
        req.n = params.image_count;
        req.model = generationModel; // TODO: make change model

        co_return co_await api_.imagesGeneration(std::move(req));
    }

    utils::AsyncResult<dto::ImageResponse> edit(Params &params)
    {
        dto::EditImageRequest req;
        req.prompt = std::move(params.prompt);
        req.n = params.image_count;
        req.model = editionModel; // TODO: make change model

        for (const auto &id : params.image_ids)
        {
            const std::string *imageBin = imgStore_.findImageById(id);
            if (!imageBin)
                co_return std::unexpected(openai::Error::UnknownImageId);

            auto base64Pr = utils::Base64::encode(*imageBin);
            if (!base64Pr)
                co_return std::unexpected(base64Pr.error());

            dto::ImageReference ref;
            ref.image_url = openai::HistoryUtils::getBase64JpegPrefix() + base64Pr.value();
            req.images.push_back(std::move(ref));
        }

        co_return co_await api_.imagesEdit(std::move(req));
    }
};
} // namespace tools