#pragma once

#include "containers/storage/storage.hpp"

#include <array>


template <pool_item_derived T, uint32_t N>
class static_storage
{
    template <template <typename> storage, typename T>
    friend class orchestrator;
    
public:
    using tag = storage_tag(storage_grow::fixed, storage_layout::continuous);

    static_storage();

    template <typename... Args>
    T* push(Args&&... args);
    T* push(T* object);

    template <typename... Args>
    void pop(T* obj, Args&&... args);

private:
    void release(T* obj);

private:
    std::array<T, N> _data;
    T* _current;
};


template <pool_item_derived T, uint32_t N>
static_storage<T, N>::static_storage() :
    _data(),
    _current(&_data[0])
{}

template <pool_item_derived T, uint32_t N>
template <typename... Args>
T* static_storage<T, N>::push(Args&&... args)
{
    assert(_current < &_data[0] + N && "Writing out of bounds");
    _current->construct(std::forward<Args>(args)...); 
    _current->recreate_ticket();
    return _current++;
}

template <pool_item_derived T, uint32_t N>
T* static_storage<T, N>::push(T* object)
{
    assert(_current < &_data[0] + N && "Writing out of bounds");
    *_current = std::move(*object);
    return _current++;
}

template <pool_item_derived T, uint32_t N>
template <typename... Args>
void static_storage<T, N>::pop(T* obj, Args&&... args)
{
    obj->destroy(std::forward<Args>(args)...); 
    obj->invalidate_ticket();
    release(obj);
}

template <pool_item_derived T, uint32_t N>
void static_storage<T, N>::release(T* obj)
{
    *obj = std::move(*_current--);
}
