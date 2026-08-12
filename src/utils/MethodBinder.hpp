#pragma once

#include <utility>

namespace utils
{
template <class Class, typename Return, typename... Args>
constexpr auto buildMethod(Return (Class::*method)(Args...), Class *self) noexcept
{
    return [self, method](Args &&...args) noexcept(noexcept((self->*method)(std::forward<Args...>(args...)))) -> Return
    {
        return (self->*method)(std::forward<Args...>(args...));
    };
}
template <class Class, typename Return, typename... Args>
constexpr auto buildMethod(Return (Class::*method)(Args...) const, const Class *self) noexcept
{
    return [self, method](Args &&...args) noexcept(noexcept((self->*method)(std::forward<Args...>(args...)))) -> Return
    {
        return (self->*method)(std::forward<Args...>(args...));
    };
}
template <class Class, typename Return, typename... Args>
constexpr auto buildMethod(Return (Class::*method)(Args...), Class &self) noexcept
{
    return [self = &self, method](Args &&...args) noexcept(noexcept((self->*method)(std::forward<Args...>(args...)))) -> Return
    {
        return (self->*method)(std::forward<Args...>(args...));
    };
}
template <class Class, typename Return, typename... Args>
constexpr auto buildMethod(Return (Class::*method)(Args...) const, const Class &self) noexcept
{
    return [self = &self, method](Args &&...args) noexcept(noexcept((self->*method)(std::forward<Args...>(args...)))) -> Return
    {
        return (self->*method)(std::forward<Args...>(args...));
    };
}

} // namespace utils