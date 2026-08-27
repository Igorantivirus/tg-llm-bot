#pragma once

#include <string>
#include <vector>

namespace utils
{

class StringUtils
{
public:
    static std::vector<std::string_view> splitArgs(const std::string &str)
    {
        std::vector<std::string_view> res;

        std::size_t begin = 0;
        for (std::size_t end = 0; end < str.size(); ++end)
        {
            bool begSpace = isSpaceSymbol(str[begin]);
            bool endSpace = isSpaceSymbol(str[end]);
            if (endSpace && !begSpace) // Нашли
            {
                res.push_back(std::string_view(str.begin() + begin, str.begin() + end));
                begin = end;
            }
            else if (begSpace && !endSpace)
                begin = end;
        }
        if (!isSpaceSymbol(str[begin]))
            res.push_back(std::string_view(str.begin() + begin, str.end()));
        return res;
    }

    inline static constexpr bool isSpaceSymbol(const char c)
    {
        return c == ' ' || c == '\t' || c == '\n';
    }

private:
};

} // namespace utils