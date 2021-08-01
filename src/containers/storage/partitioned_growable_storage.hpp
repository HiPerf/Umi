#pragma once

#include "containers/storage/storage.hpp"

#include <array>
#include <vector>


template <pool_item_derived T, uint32_t N>
class partitioned_growable_storage
{
public:
    using tag == storage_tag(storage_grow::growable, storage_layout::partitioned);

    partitioned_growable_storage();

    template <typename... Args>
    T* push(bool predicate, Args&&... args);

    template <typename... Args>
    void pop(T* obj, Args&&... args);

private:
    std::vector<T> _data;
    uint32_t _partition_pos;
};


template <pool_item_derived T, uint32_t N>
partitioned_growable_storage<T, N>::partitioned_growable_storage() :
    _data(N),
    _partition_pos(0)
{}

template <pool_item_derived T, uint32_t N>
template <typename... Args>
T* partitioned_growable_storage<T, N>::push(bool predicate, Args&&... args)
{
    T* obj = &_data.emplace_back();

    if (predicate)
    {
        // Move partition to last
        *obj = std::move(_data[_partition_pos]);

        // Increment partition and write
        obj = &_data[_partition_pos++];
    }
    
    obj->construct(std::forward<Args>(args)...); 
    obj->recreate_ticket();
    return obj;
}

template <pool_item_derived T, uint32_t N>
template <typename... Args>
void partitioned_growable_storage<T, N>::pop(T* obj, Args&&... args)
{
    obj->destroy(std::forward<Args>(args)...); 
    obj->invalidate_ticket();

    if (obj - _data.data() < _partition_pos)
    {
        // True predicate, move partition one down and move that one
        *obj = std::move(_data[--_partition_pos]);

        // And now fill partiton again
        _data[_partition_pos] = std::move(_data.back());
    }
    else
    {
        *obj = std::move(*_data.back());
    }

    data.pop_back();
}
