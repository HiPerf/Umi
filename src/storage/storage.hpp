#pragma once

#include "containers/pool_item.hpp"

#include <atomic>
#include <inttypes.h>


template <typename T>
class entity;

template <typename T>
concept pool_item_derived = std::is_base_of_v<pool_item<T>, T> || (
    std::is_base_of_v<pool_item<entity<T>>, T> && std::is_base_of_v<entity<T>, T>);


enum class storage_grow : uint8_t
{
    none            = 0,
    fixed           = 1,
    growable        = 2
};

enum class storage_layout : uint8_t
{
    none            = 0,
    continuous      = 1,
    partitioned     = 2
};

inline constexpr uint8_t storage_tag(storage_grow grow, storage_layout layout)
{
    return (static_cast<uint8_t>(grow) << 4) | static_cast<uint8_t>(layout);
}

inline constexpr bool has_storage_tag(uint8_t tag, storage_grow grow, storage_layout layout)
{
    return tag & ((static_cast<uint8_t>(grow) << 4) | static_cast<uint8_t>(layout));
}


template <template <typename, uint32_t> typename storage, typename T, uint32_t N>
class orchestrator
{
public:
    static constexpr inline uint8_t tag = storage<T, N>::tag;

    T* get(uint64_t id) const noexcept;

    template <typename... Args>
    T* push(Args&&... args) noexcept;
    void pop(T* obj) noexcept;

    void clear() noexcept;
    
    template <template <typename, uint32_t> typename other_storage, uint32_t M>
    T* move(other_storage<T, M>& other, T* obj) noexcept;

    inline auto range() noexcept
    {
        return _storage.range();
    }

    template <typename = std::enable_if_t<has_storage_tag(tag, storage_grow::none, storage_layout::partitioned)>>
    inline auto range_until_partition() noexcept
    {
        return _storage.range_until_partition();
    }
    
    template <typename = std::enable_if_t<has_storage_tag(tag, storage_grow::none, storage_layout::partitioned)>>
    inline auto range_from_partition() noexcept
    {
        return _storage.range_from_partition();
    }

    inline uint32_t size() const noexcept;
    inline bool empty() const noexcept;
    inline bool full() const noexcept;

private:
    std::unordered_map<uint64_t, typename ::ticket<entity<typename T::derived_t>>::ptr> _tickets;
    storage<T, N> _storage;
};

template <template <typename, uint32_t> typename storage, typename T, uint32_t N>
T* orchestrator<storage, T, N>::get(uint64_t id) const noexcept
{
    if (auto it = _tickets.find(id); it != _tickets.end())
    {
        if (auto ticket = it->second; ticket->valid())
        {
            return ticket->get()->derived();
        }
    }

    return nullptr;
}

template <template <typename, uint32_t> typename storage, typename T, uint32_t N>
template <typename... Args>
T* orchestrator<storage, T, N>::push(Args&&... args) noexcept
{
    T* obj = _storage.push(std::forward<Args>(args)...);
    _tickets.emplace(obj->id(), obj->ticket());
    return obj;
}

template <template <typename, uint32_t> typename storage, typename T, uint32_t N>
void orchestrator<storage, T, N>::pop(T* obj) noexcept
{
    _tickets.erase(obj->id());
    _storage.pop(obj);
}

template <template <typename, uint32_t> typename storage, typename T, uint32_t N>
void orchestrator<storage, T, N>::clear() noexcept
{
    _tickets.clear();
    _storage.clear();
}

template <template <typename, uint32_t> typename storage, typename T, uint32_t N>
template <template <typename, uint32_t> typename other_storage, uint32_t M>
T* orchestrator<storage, T, N>::move(other_storage<T, M>& other, T* obj) noexcept
{
    // Change vectors
    T* new_ptr = other.push(obj);
    _storage.release(obj);
    
    // Add to dicts
    _tickets.erase(new_ptr->id());
    other._tickets.emplace(new_ptr->id(), new_ptr->ticket());
    return new_ptr;
}

template <template <typename, uint32_t> typename storage, typename T, uint32_t N>
inline uint32_t orchestrator<storage, T, N>::size() const noexcept
{
    return _storage.size();
}

template <template <typename, uint32_t> typename storage, typename T, uint32_t N>
inline bool orchestrator<storage, T, N>::empty() const noexcept
{
    return size() == 0;
}

template <template <typename, uint32_t> typename storage, typename T, uint32_t N>
inline bool orchestrator<storage, T, N>::full() const noexcept
{
    return _storage.full();
}
