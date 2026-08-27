#pragma once

#include <concepts>
#include <stdexcept>

#include <utils/Jsonser.hpp>

namespace utils
{

template <typename T>
class NonNullCopybleUniquePtr
{
public:
    // Стандартные синтаксические конструкторы и деструктор
    NonNullCopybleUniquePtr()
        : ptr_(new T{})
    {
    }
    NonNullCopybleUniquePtr(const NonNullCopybleUniquePtr &other)
        : ptr_(new T{*other.ptr_})
    {
    }
    NonNullCopybleUniquePtr(NonNullCopybleUniquePtr &&other) noexcept
        : ptr_(other.ptr_)
    {
        other.ptr_ = nullptr;
    }
    ~NonNullCopybleUniquePtr()
    {
        delete ptr_; // nullptr удалять безопасно
        ptr_ = nullptr;
    }
    // Конструкторы по типу
    NonNullCopybleUniquePtr(const T &other)
        : ptr_(new T{other})
    {
    }
    NonNullCopybleUniquePtr(T &&other) noexcept
        : ptr_(new T{std::move(other)})
    {
    }
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    NonNullCopybleUniquePtr(Args &&...args)
        : ptr_(new T{std::forward<Args>(args)...})
    {
    }
    // Операторы присваивания
    NonNullCopybleUniquePtr &operator=(const NonNullCopybleUniquePtr &other)
    {
        checkNullable(&other);
        if (this == &other)
            return *this;
        renew(new T{*other.ptr_});
        return *this;
    }
    NonNullCopybleUniquePtr &operator=(NonNullCopybleUniquePtr &&other) noexcept
    {
        checkNullable(&other);
        if (this == &other)
            return *this;
        renew(new T{std::move(*other.ptr_)});
        other.ptr_ = nullptr;
        return *this;
    }

    const T &get() const
    {
        checkNullable(this);
        return *ptr_;
    }
    T &get()
    {
        checkNullable(this);
        return *ptr_;
    }

    const T &operator*() const
    {
        return get();
    }
    T &operator*()
    {
        return get();
    }
    const T *operator->() const
    {
        checkNullable(this);
        return ptr_;
    }
    T *operator->()
    {
        checkNullable(this);
        return ptr_;
    }

    template <jsonser::BasicJson J, typename Type>
    friend void to_json(J &j, const NonNullCopybleUniquePtr<Type> &ptr)
    {
        jsonser::Serialize::toJson<J, Type>(j, ptr.get());
    }
    template <jsonser::BasicJson J, typename Type>
    friend void from_json(const J &j, NonNullCopybleUniquePtr<Type> &ptr)
    {
        jsonser::Deserialize::fromJson<J, Type>(j, ptr.get());
    }

private:
    T *ptr_;\

private:
    static void checkNullable(const NonNullCopybleUniquePtr<T> *self)
    {
        if (!self->ptr_)
            throw std::logic_error("NonNullCopybleUniquePtr is destroyed.");
    }

    void renew(T *ptr)
    {
        if (ptr_)
            delete ptr_;
        ptr_ = ptr;
    }
};

} // namespace utils