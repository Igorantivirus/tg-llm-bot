#pragma once

namespace utils
{
class BusyGuard
{
public:
    BusyGuard(bool &busy)
        : busy_(&busy)
    {
        *busy_ = true;
    }
    ~BusyGuard()
    {
        if (*busy_)
            *busy_ = false;
    }

    BusyGuard(const BusyGuard &) = delete;
    BusyGuard(BusyGuard &&) = delete;
    BusyGuard &operator=(const BusyGuard &) = delete;
    BusyGuard &operator=(BusyGuard &&) = delete;

private:
    bool *busy_;
};
} // namespace utils