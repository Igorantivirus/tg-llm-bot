#pragma once

#include <string>
#include <vector>

namespace utils
{

class StringUtils
{
public:
    template <typename String>
    static std::vector<String> splitArgs(const std::string_view str)
    {
        if (str.empty())
            return {};
        std::vector<String> res;

        std::size_t begin = 0;
        for (std::size_t end = 0; end < str.size(); ++end)
        {
            bool begSpace = isSpaceSymbol(str[begin]);
            bool endSpace = isSpaceSymbol(str[end]);
            if (endSpace && !begSpace) // Нашли
            {
                res.push_back(String(str.begin() + begin, str.begin() + end));
                begin = end;
            }
            else if (begSpace && !endSpace)
                begin = end;
        }
        if (!isSpaceSymbol(str[begin]))
            res.push_back(String(str.begin() + begin, str.end()));
        return res;
    }
    static std::size_t findSpaceSymbol(const std::string_view str, const std::size_t begin = 0)
    {
        for (std::size_t i = begin; i < str.size(); ++i)
            if (isSpaceSymbol(str[i]))
                return i;
        return std::string::npos;
    }
    inline static constexpr bool isSpaceSymbol(const char c)
    {
        return c == ' ' || c == '\t' || c == '\n';
    }

private:
};

} // namespace utils