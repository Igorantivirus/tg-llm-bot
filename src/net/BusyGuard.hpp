#pragma once

namespace net::details
{
class BusyGuard
{
public:
    BusyGuard(bool &busy)
        : busy_(&busy)
    {
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
} // namespace net::details