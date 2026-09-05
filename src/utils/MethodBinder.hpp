#pragma once

#include <utility>

namespace utils
{
template <class Class, typename Return, typename... Args>
constexpr auto buildMethod(Return (Class::*method)(Args...), Class *self) noexcept
{
    return [self, method]<typename... Ts>(Ts &&...args) noexcept(noexcept((self->*method)(std::forward<Ts>(args)...))) -> Return
    {
        return (self->*method)(std::forward<Ts>(args)...);
    };
}
template <class Class, typename Return, typename... Args>
constexpr auto buildMethod(Return (Class::*method)(Args...) const, const Class *self) noexcept
{
    return [self, method]<typename... Ts>(Ts &&...args) noexcept(noexcept((self->*method)(std::forward<Ts>(args)...))) -> Return
    {
        return (self->*method)(std::forward<Ts>(args)...);
    };
}
template <class Class, typename Return, typename... Args>
constexpr auto buildMethod(Return (Class::*method)(Args...), Class &self) noexcept
{
    return [self = &self, method]<typename... Ts>(Ts &&...args) noexcept(noexcept((self->*method)(std::forward<Ts>(args)...))) -> Return
    {
        return (self->*method)(std::forward<Ts>(args)...);
    };
}
template <class Class, typename Return, typename... Args>
constexpr auto buildMethod(Return (Class::*method)(Args...) const, const Class &self) noexcept
{
    return [self = &self, method]<typename... Ts>(Ts &&...args) noexcept(noexcept((self->*method)(std::forward<Ts>(args)...))) -> Return
    {
        return (self->*method)(std::forward<Ts>(args)...);
    };
}

} // namespace utils