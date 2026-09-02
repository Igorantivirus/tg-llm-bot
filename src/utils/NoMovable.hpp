#pragma once

namespace utils
{

class NoMovable
{
public:
    virtual ~NoMovable() = default;

    NoMovable() = default;

    NoMovable(const NoMovable &) = delete;
    NoMovable(NoMovable &&) = delete;

    NoMovable &operator=(const NoMovable &) = delete;
    NoMovable &operator=(NoMovable &&) = delete;
};

} // namespace utils