#pragma once

#include "containers/pool_item.hpp"

#include <inttypes.h>


template<typename T>
concept pool_item_derived = std::is_base_of_v<pool_item<T>, T>;


enum class storage_grow
{
    fixed           = 1,
    growable        = 2
};

enum class storage_layout
{
    continuous      = 1,
    partitioned     = 2
};

inline constexpr uint8_t storage_tag(storage_grow grow, storage_layout layout)
{
    return (grow << 4) | layout;
}


template <template <typename> storage, typename T>
class orchestrator
{
public:
    using tag = storage<T>::tag;

    template <typename... Args>
    T* push(Args&&... args);
    void pop(T* obj);
    
    template <template <typename> other_storage>
    T* move(other_storage<T>& other, T* obj);

private:
    std::unordered_map<uint64_t, ::ticket<T>::ptr> _tickets;
    storage<T> _storage;
};


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
    T* new_ptr = other.push(obj);
    _storage.release(obj);
    return new_ptr;
}
