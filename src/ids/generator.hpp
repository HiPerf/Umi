#pragma once

#include <atomic>
#include <inttypes.h>


class generator
{
public:
    constexpr generator() noexcept;

    inline constexpr uint64_t peek() const noexcept;
    inline constexpr uint64_t next() noexcept;

private:
    // Current next id
    std::atomic<uint64_t> _current;
};


constexpr generator::generator() noexcept:
    _current(0)
{
    generators_collection<T>::store(this);
}

inline constexpr uint64_t generator::peek() const noexcept
{
    return _current;
}

inline constexpr uint64_t generator::next() noexcept
{
    return _current++;
}
