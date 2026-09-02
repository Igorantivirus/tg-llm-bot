#pragma once

#include "dto/ChatCompletions/JsonSchema.hpp"
#include "utils/Types.hpp"
#include <dto/ChatCompletions/Tools.hpp>
#include <memory>
#include <string>

namespace openai
{

class Tool
{
public:
    using Ptr = std::unique_ptr<Tool>;

public:
    virtual ~Tool() = default;

    virtual utils::AsyncResult<std::string> run(std::string args) = 0;

    virtual std::string         name() const = 0;
    virtual std::string         description() const = 0;
    virtual std::optional<bool> strict() const
    {
        return std::nullopt;
    }
    virtual std::optional<dto::schema::Object> parameters() const
    {
        return std::nullopt;
    }

    dto::Tool dto()
    {
        dto::Tool res;
        res.function = dto::ToolFunction{.name = name(), .description = description(), .parameters = parameters(), .strict = strict()};
        return res;
    }
};

} // namespace openai