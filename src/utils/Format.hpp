#pragma once

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace utils
{
class Format
{
public:
    template <typename... Args>
    static std::string format(const std::string &fmt, Args &&...args)
    {
        std::string result;
        size_t      pos = 0;
        size_t      next;

        const std::vector<std::string> values{to_string_impl(std::forward<Args>(args))...};
        const size_t                   N = values.size();
        size_t                         idx = 0;

        // последовательно ищем все "{}"
        while ((next = fmt.find("{}", pos)) != std::string::npos)
        {
            result += fmt.substr(pos, next - pos); // добавляем текст до маркера
            if (idx >= N)
                throw std::runtime_error("format: not enough arguments");
            result += values[idx]; // подставляем аргумент
            ++idx;
            pos = next + 2; // пропускаем "{}"
        }

        // добавляем остаток строки
        result += fmt.substr(pos);

        if (idx < N)
            throw std::runtime_error("format: too many arguments");

        return result;
    }

private:
    template <typename T>
    static std::string to_string_impl(const T &value)
    {
        if constexpr (std::is_same_v<std::decay_t<T>, std::string>)
            return value; // уже строка
        else if constexpr (std::is_same_v<std::decay_t<T>, const char *>)
            return std::string(value);
        else if constexpr (std::is_same_v<std::decay_t<T>, char>)
            return std::string(1, value);
        else if constexpr (std::is_same_v<std::decay_t<T>, bool>)
            return value ? "true" : "false";
        else if constexpr (std::is_arithmetic_v<std::decay_t<T>>)
            return std::to_string(value);
        else
        {
            std::ostringstream oss;
            oss << value;
            return oss.str();
        }
    }
};
} // namespace utils