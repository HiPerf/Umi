#pragma once

#include "storage/storage.hpp"

#include <array>
#include <vector>


template <pool_item_derived T, uint32_t N>
class partitioned_growable_storage
{
    template <template <typename, uint32_t> typename storage, typename T, uint32_t N>
    friend class orchestrator;
    
public:
    static constexpr inline uint8_t tag = storage_tag(storage_grow::growable, storage_layout::partitioned);

    using base_t = entity<T>;
    using derived_t = T;

    partitioned_growable_storage() noexcept;
    ~partitioned_growable_storage() noexcept;

    template <typename... Args>
    T* push(bool predicate, Args&&... args) noexcept;
    T* push(bool predicate, T* object) noexcept;

    template <typename... Args>
    void pop(T* obj, Args&&... args) noexcept;

    void clear() noexcept;
    
    inline auto range() noexcept
    {
        return ranges::views::transform(
            _data,
            [](T& obj) { return &obj; });
    }
    
    inline auto range_until_partition() noexcept
    {
        return ranges::views::transform(
            ranges::views::slice(_data, 0, static_cast<std::size_t>(_partition_pos)),
            [](T& obj) { return &obj; });
    }
    
    inline auto range_from_partition() noexcept
    {
        return ranges::views::transform(
            ranges::views::slice(_data, static_cast<std::size_t>(_partition_pos), _data.size()),
            [](T& obj) { return &obj; });
    }

    inline uint32_t size() const noexcept;
    inline bool empty() const noexcept;
    inline bool full() const noexcept;

private:
    void release(T* obj) noexcept;

private:
    std::vector<T> _data;
    uint32_t _partition_pos;
};


template <pool_item_derived T, uint32_t N>
partitioned_growable_storage<T, N>::partitioned_growable_storage() noexcept :
    _data(),
    _partition_pos(0)
{
    _data.reserve(N);
}

template <pool_item_derived T, uint32_t N>
partitioned_growable_storage<T, N>::~partitioned_growable_storage() noexcept
{
    clear();
}

template <pool_item_derived T, uint32_t N>
template <typename... Args>
T* partitioned_growable_storage<T, N>::push(bool predicate, Args&&... args) noexcept
{
    T* obj = &_data.emplace_back();

    if (predicate)
    {
        // Move partition to last
        if (auto& candidate = _data[_partition_pos]; obj != &candidate)
        {
            *obj = std::move(candidate);
        }

        // Increment partition and write
        obj = &_data[_partition_pos++];
    }
    
    static_cast<base_t&>(*obj).construct(std::forward<Args>(args)...); 
    static_cast<base_t&>(*obj).recreate_ticket();
    return obj;
}

template <pool_item_derived T, uint32_t N>
T* partitioned_growable_storage<T, N>::push(bool predicate, T* object) noexcept
{
    T* obj = &_data.emplace_back();

    if (predicate)
    {
        // Move partition to last
        *obj = std::move(_data[_partition_pos]);

        // Increment partition and write
        obj = &_data[_partition_pos++];
    }

    *obj = std::move(*object);
    return obj;
}

template <pool_item_derived T, uint32_t N>
template <typename... Args>
void partitioned_growable_storage<T, N>::pop(T* obj, Args&&... args) noexcept
{
    static_cast<base_t&>(*obj).destroy(std::forward<Args>(args)...); 
    static_cast<base_t&>(*obj).invalidate_ticket();
    release(obj);
}

template <pool_item_derived T, uint32_t N>
void partitioned_growable_storage<T, N>::release(T* obj) noexcept
{
    if (obj - _data.data() < _partition_pos)
    {
        // True predicate, move partition one down and move that one
        if (auto& candidate = _data[--_partition_pos]; obj != &candidate)
        {
            *obj = std::move(candidate);
        }

        // And now fill partition
        // There is no need to perform a check as the obj lies on the left side of the partition,
        // while the back lies on the right
        _data[_partition_pos] = std::move(_data.back());
    }
    else if (obj != &_data.back())
    {
        *obj = std::move(_data.back());
    }

    _data.pop_back();
}

template <pool_item_derived T, uint32_t N>
void partitioned_growable_storage<T, N>::clear() noexcept
{
    for (auto& obj : _data)
    {
        static_cast<base_t&>(obj).destroy();
        static_cast<base_t&>(obj).invalidate_ticket();
    }

    _data.clear();
}

template <pool_item_derived T, uint32_t N>
inline uint32_t partitioned_growable_storage<T, N>::size() const noexcept
{
    return _data.size();
}

template <pool_item_derived T, uint32_t N>
inline bool partitioned_growable_storage<T, N>::empty() const noexcept
{
    return size() == 0;
}

template <pool_item_derived T, uint32_t N>
inline bool partitioned_growable_storage<T, N>::full() const noexcept
{
    return false;
}

