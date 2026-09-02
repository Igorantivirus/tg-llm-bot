#pragma once

#include <expected>
#include <optional>
#include <stdexcept>

#include "Error.hpp"
#include "Types.hpp"

namespace utils
{
template <typename T>
class StreamGenerator
{
public:
    static constexpr std::unexpected<Error> endOfStream = std::unexpected(Error::EndOfStreamData);

public:
    virtual ~StreamGenerator() = default;

    bool done() const
    {
        return endReason_.has_value() || !isReady();
    }
    bool isEndOfStream() const
    {
        return done() && endReasonIsEndOfStream();
    }
    bool isError() const
    {
        return done() && !endReasonIsEndOfStream();
    }
    ErrorCode endReason() const
    {
        if (!done())
            throw std::logic_error("The stream has not finished yet.");
        return endReason_.value();
    }

    AsyncResult<T> next()
    {
        if (done())
            co_return std::unexpected(endReason_.value());
        auto nextValue = co_await nextImpl();
        if (!nextValue)
            processError(nextValue.error());
        co_return nextValue;
    }

protected:
    virtual void close()
    {
    }
    virtual void reset() // Если пользователь определяет ресет - надо вызвать этот ресет
    {
        endReason_.reset();
    }
    virtual bool isReady() const
    {
        return true;
    }

    // При вызове гарантируется valid (если close и valid синхронизированы)
    virtual AsyncResult<T> nextImpl() = 0;

private:
    std::optional<ErrorCode> endReason_ = std::nullopt;

    void processError(ErrorCode err)
    {
        close();
        endReason_.emplace(err);
    }
    bool endReasonIsEndOfStream() const
    {
        return endReason_.has_value() && endReason_.value() == Error::EndOfStreamData;
    }
};

} // namespace utils