#pragma once

#include "openai/Tools/ToolResult.hpp"
#include "openai/api/Api.hpp"
#include "openai/dto/ChatCompletions/JsonSchema.hpp"
#include "utils/NonNullCopybleUniquePtr.hpp"
#include <memory>
#include <openai/Tools/Tool.hpp>
#include <utils/Parser.hpp>

namespace openai
{
class CreateImageToolResult : public ToolResult
{
public:
    static inline const std::string calledFunctionName = "create_image";

public:
    CreateImageToolResult(dto::ImageResponse dto)
        : ToolResult(calledFunctionName), dto_(std::move(dto))
    {
    }

    std::string toString() const override
    {
        auto resp = utils::serialize(dto_);
        if (!resp)
            return resp.error().message();
        return resp.value();
    }

    const dto::ImageResponse &getDto() const
    {
        return dto_;
    }

private:
    dto::ImageResponse dto_;
};

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
        std::string     prompt;
        ActionType      action = ActionType::generate;
        AspectRatioType aspect_ratio = AspectRatioType::square;
        unsigned short  image_count = 1;
    };

public:
    CreateImage(Api &api)
        : api_(api)
    {
    }

    utils::AsyncResult<ToolResult::Ptr> run(std::string args) override
    {
        auto dto = utils::deserialize<Params>(args);
        if (!dto)
            co_return std::unexpected(dto.error());
        Params params = std::move(dto.value());

        dto::GenerateImageRequest req;
        req.prompt = std::move(params.prompt);
        req.n = params.image_count;
        req.model = "qwen-edit-nsfw"; // TODO: make change model

        auto res = co_await api_.imagesGeneration(std::move(req));
        if (!res)
            co_return std::unexpected(res.error());

        co_return std::make_shared<CreateImageToolResult>(std::move(res.value()));
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