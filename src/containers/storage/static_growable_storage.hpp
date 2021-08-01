#pragma once

#include "containers/storage/storage.hpp"

#include <array>
#include <vector>


template <pool_item_derived T, uint32_t N>
class static_growable_storage
{
public:
    using tag == storage_tag(storage_grow::mixed, storage_layout::continuous);

    static_growable_storage();

    template <typename... Args>
    T* push(Args&&... args);

    template <typename... Args>
    void pop(T* obj, Args&&... args);

private:
    bool is_static(T* obj) noexcept const;
    bool is_static_full() noexcept const;

private:
    std::array<T, N> _data;
    T* _current;
    std::vector<T> _growable;
};


template <pool_item_derived T, uint32_t N>
static_growable_storage<T, N>::static_growable_storage() :
    _data(),
    _current(&_data[0]),
    _growable(N)
{}

template <pool_item_derived T, uint32_t N>
bool static_growable_storage<T, N>::is_static(T* obj) noexcept const
{
    return obj >= &_data[0] + N;
}

template <pool_item_derived T, uint32_t N>
bool static_growable_storage<T, N>::is_static_full() noexcept const
{
    return _current == &_data[0] + N;
}

template <pool_item_derived T, uint32_t N>
template <typename... Args>
T* static_growable_storage<T, N>::push(Args&&... args)
{
    T* obj = _current;
    if (!is_static_full())
    {
        assert(_current < &_data[0] + N && "Writing out of bounds");
        ++_current;
    }
    else
    {
        obj = &_growable.emplace_back();
    }

    obj->construct(std::forward<Args>(args)...); 
    obj->recreate_ticket();
    return obj;
}

template <pool_item_derived T, uint32_t N>
template <typename... Args>
void static_growable_storage<T, N>::pop(T* obj, Args&&... args)
{
    obj->destroy(std::forward<Args>(args)...); 
    obj->invalidate_ticket();
    
    if (is_static(obj))
    {
        *obj = std::move(*--_current);
    }
    else
    {
        std::move(*obj, _growable.back());
        _growable.pop_back();
    }
}
