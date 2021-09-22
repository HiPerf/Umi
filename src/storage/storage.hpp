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

inline constexpr uint8_t storage_tag(storage_grow grow, storage_layout layout) noexcept
{
    return (static_cast<uint8_t>(grow) << 4) | static_cast<uint8_t>(layout);
}

inline constexpr bool has_storage_tag(uint8_t tag, storage_grow grow, storage_layout layout) noexcept
{
    return tag & ((static_cast<uint8_t>(grow) << 4) | static_cast<uint8_t>(layout));
}

inline constexpr bool is_partitioned_storage(uint8_t tag) noexcept
{
    return has_storage_tag(tag, storage_grow::none, storage_layout::partitioned);
}

template <template <typename, uint32_t> typename storage, typename T, uint32_t N>
class orchestrator
{
    template <template <typename, uint32_t> typename S, typename D, uint32_t M>
    friend class orchestrator;

public:
    static constexpr inline uint8_t tag = storage<T, N>::tag;

    using base_t = typename storage<T, N>::base_t;
    using derived_t = typename storage<T, N>::derived_t;
    using orchestrator_t = orchestrator<storage, T, N>;
    
    orchestrator() noexcept;

    orchestrator(orchestrator&& other) noexcept = default;
    orchestrator& operator=(orchestrator && other) noexcept = default;

    T* get(uint64_t id) const noexcept;

    template <typename... Args>
    T* push(Args&&... args) noexcept;
    void pop(T* obj) noexcept;

    void clear() noexcept;
    
    template <template <typename, uint32_t> typename S, uint32_t M, typename... Args>
    T* move(orchestrator<S, T, M>& other, T* obj, Args... args) noexcept;

    inline auto range() noexcept
    {
        return _storage.range();
    }

    template <typename D = storage<T, N>, typename = std::enable_if_t<has_storage_tag(D::tag, storage_grow::none, storage_layout::partitioned)>>
    inline auto range_until_partition() noexcept
    {
        return _storage.range_until_partition();
    }
    
    template <typename D = storage<T, N>, typename = std::enable_if_t<has_storage_tag(D::tag, storage_grow::none, storage_layout::partitioned)>>
    inline auto range_from_partition() noexcept
    {
        return _storage.range_from_partition();
    }

    template <typename D = storage<T, N>, typename = std::enable_if_t<has_storage_tag(D::tag, storage_grow::none, storage_layout::partitioned)>>
    inline auto change_partition(bool predicate, T* obj) noexcept
    {
        return _storage.change_partition(predicate, obj);
    }

    inline uint32_t size() const noexcept;
    inline bool empty() const noexcept;
    inline bool full() const noexcept;

    inline storage<T, N>& raw_storage() noexcept;

private:
    std::unordered_map<uint64_t, typename ::ticket<entity<typename T::derived_t>>::ptr> _tickets;
    storage<T, N> _storage;
};

template <template <typename, uint32_t> typename storage, typename T, uint32_t N>
orchestrator<storage, T, N>::orchestrator() noexcept :
    _tickets(),
    _storage()
{}

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
template <template <typename, uint32_t> typename S, uint32_t M, typename... Args>
T* orchestrator<storage, T, N>::move(orchestrator<S, T, M>& other, T* obj, Args... args) noexcept
{
    // Change vectors
    T* new_ptr = nullptr;
    if constexpr (has_storage_tag(orchestrator<S, T, M>::tag, storage_grow::none, storage_layout::partitioned))
    {
        if constexpr (has_storage_tag(tag, storage_grow::none, storage_layout::partitioned))
        {
            new_ptr = other.raw_storage().push_ptr(_storage.partition(obj), obj);
            raw_storage().release(obj);
        }
        else
        {
            // Moving from a non-partitioned storage to another, what should we do?
            static_assert(sizeof...(Args) == 1, "Must provide a boolean parameter indicating the partition when moving from non-partitioned to partitioned");
            auto part = std::get<0>(std::forward_as_tuple(args...));
            static_assert(std::is_same_v<std::remove_const_t<std::remove_reference_t<decltype(part)>>, bool>, "Partition must be a boolean parameter");

            new_ptr = other.raw_storage().push_ptr(part, obj);
            raw_storage().release(obj);
        }
    }
    else
    {
        new_ptr = other.raw_storage().push_ptr(obj);
        raw_storage().release(obj);
    }

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
    return _storage.empty();
}

template <template <typename, uint32_t> typename storage, typename T, uint32_t N>
inline bool orchestrator<storage, T, N>::full() const noexcept
{
    return _storage.full();
}

template <template <typename, uint32_t> typename storage, typename T, uint32_t N>
inline storage<T, N>& orchestrator<storage, T, N>::raw_storage() noexcept
{
    return _storage;
}