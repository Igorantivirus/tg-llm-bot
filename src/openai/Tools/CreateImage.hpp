#pragma once

#include "openai/api/Api.hpp"
#include "openai/dto/ChatCompletions/JsonSchema.hpp"
#include "utils/NonNullCopybleUniquePtr.hpp"
#include <openai/Tools/Tool.hpp>
#include <utils/Parser.hpp>

namespace openai
{
class CreateImage : public Tool
{
public:
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
        std::string     promt;
        ActionType      action = ActionType::generate;
        AspectRatioType aspect_ratio = AspectRatioType::square;
        unsigned short  image_count = 1;
    };

public:
    CreateImage(Api &api)
        : api_(api)
    {
    }

    utils::AsyncResult<std::string> run(std::string args) override
    {
        auto dto = utils::deserialize<Params>(args);
        if (!dto)
            co_return std::unexpected(dto.error());
        Params params = std::move(dto.value());

        dto::GenerateImageRequest req;
        req.prompt = std::move(params.promt);
        req.n = params.image_count;
        req.model = "qwen-edit-nsfw";

        auto res = co_await api_.imagesGeneration(std::move(req));
        if (!res)
            co_return std::unexpected(res.error());

        auto resp = utils::serialize(res.value());
        if (!resp)
            co_return std::unexpected(resp.error());
        co_return resp.value();
    }
    std::string name() const override
    {
        return "create_image";
    }
    std::string description() const override
    {
        return "Create image";
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
        action_str.description = "Use 'edit' to modify an existing image, 'generate' for a new one.";
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

        obj.properties = std::move(properties);

        // Required fields
        obj.required = std::vector<std::string>{"prompt"};

        return obj;
    }

private:
    Api &api_;
};
} // namespace openai