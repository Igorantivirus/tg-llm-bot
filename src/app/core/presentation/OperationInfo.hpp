#pragma once

#include <openai/ChatsSettings/Types.hpp>

namespace core
{
// МОжно наследоваться и хранить свои данные, просто потом сделать возврат
class OperationInfo
{
public:
    using Ptr = std::shared_ptr<OperationInfo>;

public:
    explicit OperationInfo(const openai::ChatIdType chatId)
        : chatId_(chatId)
    {
    }
    virtual ~OperationInfo() = default;

    const openai::ChatIdType getChatId() const
    {
        return chatId_;
    }

    template <typename Derive>
    Derive *as()
    {
        return dynamic_cast<Derive *>(this);
    }
    template <typename Derive>
    const Derive *as() const
    {
        return dynamic_cast<const Derive *>(this);
    }

private:
    openai::ChatIdType chatId_;
};

} // namespace core