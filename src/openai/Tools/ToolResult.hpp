#pragma once

#include <memory>
#include <string>

namespace openai
{
class ToolResult
{
public:
    using Ptr = std::shared_ptr<ToolResult>;

public:
    ToolResult(std::string calledFunctionName)
        : calledFunctionName_(calledFunctionName)
    {
    }
    virtual ~ToolResult() = default;

    const std::string &calledFunction() const
    {
        return calledFunctionName_;
    }
    virtual std::string toString() const = 0;

    template <typename T>
    T *to() 
    {
        return dynamic_cast<T *>(this);
    }
    template <typename T>
    const T *to() const
    {
        return dynamic_cast<T *>(this);
    }

private:
    const std::string calledFunctionName_;
};
} // namespace openai