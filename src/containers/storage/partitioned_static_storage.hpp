#pragma once

#include "containers/storage/storage.hpp"

#include <array>


template <pool_item_derived T, uint32_t N>
class partitioned_static_storage
{
    template <template <typename> storage, typename T>
    friend class orchestrator;
    
public:
    using tag = storage_tag(storage_grow::fixed, storage_layout::partitioned);

    partitioned_static_storage();

    template <typename... Args>
    T* push(bool predicate, Args&&... args);
    T* push(bool predicate, T* object);

    template <typename... Args>
    void pop(T* obj, Args&&... args);

private:
    void release(T* obj);

private:
    std::array<T, N> _data;
    T* _current;
    T* _partition;
};


template <pool_item_derived T, uint32_t N>
partitioned_static_storage<T, N>::partitioned_static_storage() :
    _data(),
    _current(&_data[0]),
    _partition(&_data[0])
{}

template <pool_item_derived T, uint32_t N>
template <typename... Args>
T* partitioned_static_storage<T, N>::push(bool predicate, Args&&... args)
{
    assert(_current < &_data[0] + N && "Writing out of bounds");
    T* obj = _current;
    if (predicate)
    {
        // Move partition to last
        *_current = std::move(*_partition);

        // Increment partition and write
        obj = _partition++;
    }

    ++_current;
    obj->construct(std::forward<Args>(args)...); 
    obj->recreate_ticket();
    return obj;
}

template <pool_item_derived T, uint32_t N>
template <typename... Args>
T* partitioned_static_storage<T, N>::push(bool predicate, T* object)
{
    assert(_current < &_data[0] + N && "Writing out of bounds");
    T* obj = _current;
    if (predicate)
    {
        // Move partition to last
        *_current = std::move(*_partition);

        // Increment partition and write
        obj = _partition++;
    }

    ++_current;
    *obj = std::move(*object);
    return obj;
}

template <pool_item_derived T, uint32_t N>
template <typename... Args>
void partitioned_static_storage<T, N>::pop(T* obj, Args&&... args)
{
    obj->destroy(std::forward<Args>(args)...); 
    obj->invalidate_ticket();
    release(obj);
}

template <pool_item_derived T, uint32_t N>
void partitioned_static_storage<T, N>::release(T* obj)
{
    if (obj < _partition)
    {
        // True predicate, move partition one down and move that one
        *obj = std::move(*--_partition);

        // And now fill partiton again
        *_partition = std::move(*_current);
    }
    else
    {
        *obj = std::move(*_current);
    }

    --_current;
}
