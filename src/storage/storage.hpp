#pragma once

#include "containers/pool_item.hpp"

#include <atomic>
#include <inttypes.h>


template<typename T>
concept pool_item_derived = std::is_base_of_v<pool_item<T>, T>;


enum class storage_grow
{
    none            = 0,
    fixed           = 1,
    growable        = 2
};

enum class storage_layout
{
    none            = 0,
    continuous      = 1,
    partitioned     = 2
};

inline constexpr uint8_t storage_tag(storage_grow grow, storage_layout layout)
{
    return (grow << 4) | layout;
}

inline constexpr bool has_storage_tag(uint8_t tag, storage_grow grow, storage_layout layout)
{
    return tag & ((grow << 4) | layout);
}


template <template <typename> storage, typename T>
class orchestrator
{
public:
    using tag = storage<T>::tag;

    T* get(uint64_t id) const;

    template <typename... Args>
    T* push(Args&&... args);
    void pop(T* obj);
    
    template <template <typename> other_storage>
    T* move(other_storage<T>& other, T* obj);

    inline auto range()
    {
        return _storage.range();
    }

    template <typename = std::enable_if_t<has_storage_tag(tag, storage_grow::none, storage_layout::partitioned)>>
    inline auto range_until_partition()
    {
        return _storage.range_until_partition();
    }
    
    template <typename = std::enable_if_t<has_storage_tag(tag, storage_grow::none, storage_layout::partitioned)>>
    inline auto range_from_partition()
    {
        return _storage.range_from_partition();
    }

private:
    std::unordered_map<uint64_t, ::ticket<T>::ptr> _tickets;
    storage<T> _storage;
};

template <template <typename> storage, typename T>
T* orchestrator<storage, T>::get(uint64_t id) const
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

template <template <typename> storage, typename T>
template <typename... Args>
T* orchestrator<storage, T>::push(Args&&... args)
{
    T* obj = _storage.push(std::forward<Args>(args)...);
    _tickets.emplace(obj->id(), obj->ticket());
    return obj;
}

template <template <typename> storage, typename T>
void orchestrator<storage, T>::pop(T* obj)
{
    _tickets.erase(obj->id());
    _storage.pop(obj);
}

template <template <typename> storage, typename T>
template <template <typename> other_storage>
T* orchestrator<storage, T>::move(other_storage<T>& other, T* obj)
{
    // Change vectors
    T* new_ptr = other.push(obj);
    _storage.release(obj);
    
    // Add to dicts
    _tickets.erase(new_ptr->id());
    other._tickets.emplace(new_ptr->id(), new_ptr->ticket());
    return new_ptr;
}
