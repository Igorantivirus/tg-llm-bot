#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

#include <utils/StreamGenerator.hpp>
#include <utils/StringUtils.hpp>
#include <utils/Types.hpp>

#include "Error.hpp"
#include "HttpStreamReader.hpp"

namespace net
{
class SseParser : public utils::StreamGenerator<std::string>
{
public:
    SseParser(HttpStreamReader stream)
        : stream_(std::move(stream))
    {
    }

private:
    HttpStreamReader stream_;
    std::string      buffer_;

private:
    enum class NextFragmentRes : std::uint8_t
    {
        NotFount,
        Data,
        End,
        Invalid
    };

private:
    bool isReady() const override
    {
        return !stream_.done();
    }
    void close() override
    {
        buffer_.clear();
    }

    utils::AsyncResult<std::string> nextImpl() override
    {
        std::pair<std::string, NextFragmentRes> fragPair;

        while ((fragPair = tryFindFragment()), fragPair.second == NextFragmentRes::NotFount) // Пока не удалось найти - ищем
        {
            // Чтение фрагмента
            auto nextChunk = co_await stream_.next();
            if (!nextChunk)
                co_return std::unexpected(nextChunk.error());
            buffer_ += std::move(nextChunk.value()); // Читаем фрагмент
        }
        if (fragPair.second == NextFragmentRes::End) // Дошли до конца, говорим, что данных больше нет
            co_return (std::ignore = co_await readToTheEnd()), endOfStream;
        if (fragPair.second == NextFragmentRes::Data) // Нашли данные - возвращаем
            co_return fragPair.first;
        // Тут инвалидные данные
        co_return std::unexpected(Error::InvalidSseFragment);
    }

    // Если после "data: [DONE]" остались данные, чтоб сервер не подумал, что мы просто оборвали соединение - дочитаем вс до конца
    utils::AsyncResult<void> readToTheEnd()
    {
        while (auto next = co_await stream_.next())
            ; // Даже если ошибка - плевать, мы свой конец прочитали штатно, поэтому просто читаем
        co_return utils::empty;
    }

    std::pair<std::string, NextFragmentRes> tryFindFragment()
    {
        // Ищем все концы
        for (auto found = findFragment(buffer_); found.first != std::string::npos; found = findFragment(buffer_))
        {
            auto [endOfFrag, countEnd] = found;
            std::string_view                                       data = getData(buffer_, endOfFrag, countEnd);
            std::optional<std::pair<std::string, NextFragmentRes>> result;

            if (data.starts_with("data: [DONE]")) // Конец данных
                result = {{}, NextFragmentRes::End};
            else if (data.starts_with("data: ")) // Полезные данные
                result = {std::string(data.begin() + 6, data.end()), NextFragmentRes::Data};
            else if (!data.empty() && !data.starts_with(':')) // Не комментарий и не пустая строка - точно инвалид
                result = {{}, NextFragmentRes::Invalid};

            buffer_.erase(buffer_.begin(), buffer_.begin() + endOfFrag); // В конце обязательно чистим прочитанное
            if (result)
                return result.value();
        }
        return {{}, NextFragmentRes::NotFount};
    }

private:
    inline static std::string_view getData(const std::string &str, const std::size_t endOfFragment, const std::size_t sizeOfEnd)
    {
        std::string_view res(str.begin(), str.begin() + endOfFragment);
        res.remove_suffix(sizeOfEnd);
        std::size_t start = 0;
        while (start < res.size() && (utils::StringUtils::isSpaceSymbol(res[start])))
            ++start;
        res.remove_prefix(start);
        return res;
    }
    inline static std::pair<std::size_t, std::size_t> findFragment(const std::string &msg)
    {
        static const std::array<std::string_view, 2> endVariants = {"\n\n", "\r\n\r\n"};
        std::size_t                                  minIndex = std::string::npos;
        std::size_t                                  sizeOfPostfix = 0;
        for (const auto &var : endVariants)
            if (std::size_t index = msg.find(var); index != std::string::npos && minIndex > index + var.size())
            {
                sizeOfPostfix = var.size();
                minIndex = index + var.size();
            }
        return {minIndex, sizeOfPostfix};
    }
};
} // namespace net