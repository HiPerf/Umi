#pragma once

#include "containers/storage/storage.hpp"

#include <array>
#include <vector>


template <pool_item_derived T, uint32_t N>
class growable_storage
{
    template <template <typename> storage, typename T>
    friend class orchestrator;

public:
    using tag = storage_tag(storage_grow::growable, storage_layout::continuous);
    
    growable_storage();

    template <typename... Args>
    T* push(Args&&... args);
    T* push(T* obj);

    template <typename... Args>
    void pop(T* obj, Args&&... args);
    
    inline auto range()
    {
        return ranges::views::transform(
            _data,
            [](T& obj) { return &obj; });
    }

private:
    void release(T* obj);

private:
    std::vector<T> _data;
};


template <pool_item_derived T, uint32_t N>
growable_storage<T, N>::growable_storage() :
    _data(N)
{}

template <pool_item_derived T, uint32_t N>
template <typename... Args>
T* growable_storage<T, N>::push(Args&&... args)
{
    T* obj = &_data.emplace_back();
    obj->construct(std::forward<Args>(args)...); 
    obj->recreate_ticket();
    return obj;
}

template <pool_item_derived T, uint32_t N>
T* growable_storage<T, N>::push(T* obj)
{
    obj = &_data.emplace_back(std::move(*obj));
    return obj;
}

template <pool_item_derived T, uint32_t N>
template <typename... Args>
void growable_storage<T, N>::pop(T* obj, Args&&... args)
{
    obj->destroy(std::forward<Args>(args)...); 
    obj->invalidate_ticket();

    release(obj);
}

template <pool_item_derived T, uint32_t N>
void growable_storage<T, N>::release(T* obj)
{
    *obj = std::move(_data.back());
    _data.pop_back();
}
