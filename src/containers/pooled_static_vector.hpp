#pragma once

#include "containers/pool_item.hpp"

#include <array>
#include <inttypes.h>
#include <memory>
#include <queue>
#include <vector>

#include <boost/pool/pool.hpp>

#include <range/v3/view/concat.hpp>
#include <range/v3/view/slice.hpp>
#include <range/v3/view/transform.hpp>

#include <tao/tuple/tuple.hpp>



template <typename T, typename B, uint16_t InitialSize, typename Track=void>
class pooled_static_vector
{
    template <typename... D> friend class scheme;

public:
    using base_t = B;
    using derived_t = T;

public:
    constexpr pooled_static_vector();
    virtual ~pooled_static_vector();

    template <typename... Args>
    void clear(Args&&... args);

    template <typename... Args>
    T* alloc(Args&&... args);

    template <typename... Args>
    T* alloc_with_partition(bool p, Args&&... args);

    template <typename... Args>
    void free(T* object, Args&&... args);

    template <typename... Args>
    void free_with_partition(bool p, T* object, Args&&... args);

    T* partition_change(bool p, T* object);

    constexpr inline bool is_static(T* object) const;
    constexpr inline bool is_static_full() const;
    constexpr inline bool empty() const;
    inline size_t size() const;
    
    inline auto range()
    {
        // MSVC requires an extra transform to drop a reference that the compiler is not able to automatically do
#ifdef __unix__
        return ranges::views::concat(
            ranges::views::transform(
                ranges::views::slice(_objects, static_cast<uint16_t>(0), static_cast<std::size_t>(_current - &_objects[0])),
                [](T& obj) -> T* { return &obj; }),
            _extra);
#else
        return ranges::views::concat(
            ranges::views::transform(
                ranges::views::slice(_objects, static_cast<uint16_t>(0), static_cast<std::size_t>(_current - &_objects[0])),
                [](T& obj) -> T* { return &obj; }),
            ranges::views::transform(_extra, [](T*& obj) -> T* { return obj; }));
#endif
    }

    inline auto range_as_ref()
    {
        return ranges::views::concat(
            ranges::views::slice(_objects, static_cast<uint16_t>(0), static_cast<std::size_t>(_current - &_objects[0])),
            ranges::views::transform(
                _extra,
                [this](T* obj) mutable -> T& { return *obj; }
            )
        );
    }

protected:
    template <uint16_t OtherSize, typename R>
    T* move_impl(T* object, pooled_static_vector<T, B, OtherSize, R>& to);
    T* move_impl(T* object);

    template <uint16_t OtherSize, typename R>
    T* move_with_partition_impl(bool p, T* object, pooled_static_vector<T, B, OtherSize, R>& to);
    T* move_with_partition_impl(bool p, T* object);

private:
    void free_impl(T* object);
    void free_with_partition_impl(bool p, T* object);

    inline T* partition_ptr()
    {
        if (_partition == _end)
        {
            if (_extra.empty())
            {
                return nullptr;
            }

            return _extra[0];
        }
        else if (is_static(_partition))
        {
            return _partition;
        }
        else if (_partition == (T*)_extra.data() + _extra.size())
        {
            return nullptr;
        }

        // TODO(gpascualg): This is too hacky
        return *(T**)_partition;
    }

private:
    T* _current;
    T* _end;
    T* _partition;
    std::array<T, InitialSize> _objects;
    std::vector<T*> _extra;
    boost::pool<> _pool;
};


template <typename T, typename B, uint16_t InitialSize, typename Track>
constexpr pooled_static_vector<T, B, InitialSize, Track>::pooled_static_vector() :
    _objects(),
    _pool(sizeof(T))
{
    _current = &_objects[0];
    _end = &_objects[0] + InitialSize;
    _partition = _current;
}

template <typename T, typename B, uint16_t InitialSize, typename Track>
pooled_static_vector<T, B, InitialSize, Track>::~pooled_static_vector()
{
    clear();
}

template <typename T, typename B, uint16_t InitialSize, typename Track>
template <typename... Args>
void pooled_static_vector<T, B, InitialSize, Track>::clear(Args&&... args)
{
    // TODO(gpascualg): Destroying entities here might block too much once we have DB
    // Probably we could temporary move ourselves to the DB thread and
    // progressively add them to said DB
    for (auto object : range())
    {
        static_cast<B&>(*object).destroy(std::forward<Args>(args)...);
        object->invalidate();
    }

    // Point static to start again
    _current = &_objects[0];
    _partition = _current;

    // Clear dynamic
    _pool.purge_memory();
    _extra.clear();
}

template <typename T, typename B, uint16_t InitialSize, typename Track>
template <typename... Args>
T* pooled_static_vector<T, B, InitialSize, Track>::alloc(Args&&... args)
{
    // Get last item available
    T* object = _current;
    if (is_static_full())
    {
        void* ptr = _pool.malloc();
        object = new (ptr) T();
        _extra.push_back(object);
    }
    else
    {
        ++_current;
    }   
    
    // Get ticket to this position
    object->_ticket = typename ::ticket<B>::ptr(new ::ticket<B>(reinterpret_cast<B*>(object)));
    static_cast<B&>(*object).construct(std::forward<Args>(args)...);

    // Track it if necessary
    if constexpr (!std::is_same_v<Track, void>)
    {
        static_cast<Track&>(*this).register_alloc(tao::get<0>(tao::forward_as_tuple(args...)), object);
    }

    return object;
}

template <typename T, typename B, uint16_t InitialSize, typename Track>
template <typename... Args>
T* pooled_static_vector<T, B, InitialSize, Track>::alloc_with_partition(bool p, Args&&... args)
{
    T* object = _current;

    // Right of the partition
    if (!p)
    {
        if (is_static_full())
        {
            // Simply alloc and place it on the rightmost
            void* ptr = _pool.malloc();
            object = new (ptr) T();
            _extra.push_back(object);
        }
        else
        {
            // Same thing, place it righmost
            ++_current;
        }
    }
    else
    {
        // Check were the partition lies
        if (is_static(_partition))
        {
            // _partition can be, at most, _end - 1
            // otherwise it is considered non-static

            // Case 1) Partition is at the end
            if (_partition == _current)
            {
                // Simply increase
                ++_current;
                ++_partition;
            }
            // Case 2) It's not at the end, but static is full
            else if (is_static_full())
            {
                // Alloc in pool and move current partition there
                void* ptr = _pool.malloc();
                T* replacement = new (ptr) T();
                _extra.push_back(replacement);
                *replacement = std::move(*_partition);

                // Now alloc the object in _partition and move it
                object = _partition++;
            }
            else
            {
                // Otherwise we need to move partition to the end and alloc there
                *_current = std::move(*_partition);
                ++_current;

                // Now alloc in partition and move it one step further
                object = _partition++;
            }
        }
        else
        {
            // This can only happen if everything is positive, so just take whatever and place it
            assert(is_static_full() && "Static must be full to reach here");

            // Partition is either _end or > _extra.data()
            T* partition = partition_ptr(); // Either _extra.data() or nullptr if there's no _extra or it's at its end
            if (partition == nullptr)
            {
                // Allocate in _extra and increase
                void* ptr = _pool.malloc();
                object = new (ptr) T();
                _extra.push_back(object);

                // Increase partition position
                _partition++;
            }
            else
            {
                // Alloc in pool and move current partition there
                void* ptr = _pool.malloc();
                T* replacement = new (ptr) T();
                _extra.push_back(replacement);
                *replacement = std::move(*partition);

                // Now alloc the object in _partition and move it
                object = partition;
                _partition++;
            }

            if (_partition > _end)
            {
                // TODO(gpascualg): This is too hacky
                _partition = (T*)_extra.data();
            }
        }
    }

    // Get ticket to this position
    object->_ticket = typename ::ticket<B>::ptr(new ::ticket<B>(reinterpret_cast<B*>(object)));
    static_cast<B&>(*object).construct(std::forward<Args>(args)...);

    // Track it if necessary
    if constexpr (!std::is_same_v<Track, void>)
    {
        static_cast<Track&>(*this).register_alloc(tao::get<0>(tao::forward_as_tuple(args...)), object);
    }

    return object;
}

template <typename T, typename B, uint16_t InitialSize, typename Track>
template <typename... Args>
void pooled_static_vector<T, B, InitialSize, Track>::free(T* object, Args&&... args)
{
    // Untrack it if necessary
    if constexpr (!std::is_same_v<Track, void>)
    {
        // TODO(gpascualg): Is there always id()? Enforce it?
        static_cast<Track&>(*this).register_free(object->id());
    }

    // Destroy obect
    static_cast<B&>(*object).destroy(std::forward<Args>(args)...);
    object->invalidate();

    // Free memory
    free_impl(object);
}

template <typename T, typename B, uint16_t InitialSize, typename Track>
template <typename... Args>
void pooled_static_vector<T, B, InitialSize, Track>::free_with_partition(bool p, T* object, Args&&... args)
{
    // Untrack it if necessary
    if constexpr (!std::is_same_v<Track, void>)
    {
        // TODO(gpascualg): Is there always id()? Enforce it?
        static_cast<Track&>(*this).register_free(object->id());
    }

    // Destroy obect
    static_cast<B&>(*object).destroy(std::forward<Args>(args)...);
    object->invalidate();

    // Free memory
    free_with_partition_impl(p, object);
}

template <typename T, typename B, uint16_t InitialSize, typename Track>
constexpr inline bool pooled_static_vector<T, B, InitialSize, Track>::is_static(T* object) const
{
    return (object < _end) && (object >= &_objects[0]);
}

template <typename T, typename B, uint16_t InitialSize, typename Track>
constexpr inline bool pooled_static_vector<T, B, InitialSize, Track>::is_static_full() const
{
    return _current == _end;
}

template <typename T, typename B, uint16_t InitialSize, typename Track>
constexpr inline bool pooled_static_vector<T, B, InitialSize, Track>::empty() const
{
    return _current == &_objects[0];
}

template <typename T, typename B, uint16_t InitialSize, typename Track>
inline size_t pooled_static_vector<T, B, InitialSize, Track>::size() const
{
    return _current - &_objects[0] + _extra.size();
}

template <typename T, typename B, uint16_t InitialSize, typename Track>
template <uint16_t OtherSize, typename R>
T* pooled_static_vector<T, B, InitialSize, Track>::move_impl(T* object, pooled_static_vector<T, B, OtherSize, R>& to)
{
    // Write in destination pool and free memory
    T* new_object = to.move_impl(object);
    free_impl(object);
    return new_object;
}

template <typename T, typename B, uint16_t InitialSize, typename Track>
T* pooled_static_vector<T, B, InitialSize, Track>::move_impl(T* object)
{
    // Insert in new location
    T* new_object = _current;
    if (is_static_full())
    {
        void* ptr = _pool.malloc();
        new_object = new (ptr) T(std::move(*object));
        _extra.push_back(new_object);
    }
    else
    {
        *new_object = std::move(*object);
        ++_current;
    }   
    
    return new_object;
}


template <typename T, typename B, uint16_t InitialSize, typename Track>
template <uint16_t OtherSize, typename R>
T* pooled_static_vector<T, B, InitialSize, Track>::move_with_partition_impl(bool p, T* object, pooled_static_vector<T, B, OtherSize, R>& to)
{
    // Write in destination pool and free memory
    T* new_object = to.move_with_partition_impl(p, object);
    free_with_partition_impl(p, object);
    return new_object;
}

template <typename T, typename B, uint16_t InitialSize, typename Track>
T* pooled_static_vector<T, B, InitialSize, Track>::move_with_partition_impl(bool p, T* object)
{
    // Insert in new location
    T* new_object = _current;

    // Right of the partition
    if (!p)
    {
        if (is_static_full())
        {
            // Simply alloc and place it on the rightmost
            void* ptr = _pool.malloc();
            new_object = new (ptr) T(std::move(*object));
            _extra.push_back(new_object);
        }
        else
        {
            // Same thing, place it righmost
            ++_current;
            *new_object = std::move(*object);
        }
    }
    else
    {
        // Check were the partition lies
        if (is_static(_partition))
        {
            // _partition can be, at most, _end - 1
            // otherwise it is considered non-static

            // Case 1) Partition is at the end
            if (_partition == _current)
            {
                // Simply increase
                ++_current;
                ++_partition;
                *new_object = std::move(*object);
            }
            // Case 2) It's not at the end, but static is full
            else if (is_static_full())
            {
                // Alloc in pool and move current partition there
                void* ptr = _pool.malloc();
                T* replacement = new (ptr) T();
                _extra.push_back(replacement);
                *replacement = std::move(*_partition);

                // Now alloc the object in _partition and move it
                *_partition = std::move(*object);
                new_object = _partition++;
            }
            else
            {
                // Otherwise we need to move partition to the end and alloc there
                *_current = std::move(*_partition);
                ++_current;

                // Now alloc in partition and move it one step further
                *_partition = std::move(*object);
                new_object = _partition++;
            }
        }
        else
        {
            // This can only happen if everything is positive, so just take whatever and place it
            assert(is_static_full() && "Static must be full to reach here");

            // Partition is either _end or > _extra.data()
            T* partition = partition_ptr(); // Either _extra.data() or nullptr if there's no _extra or it's at its end
            if (partition == nullptr)
            {
                // Allocate in _extra and increase
                void* ptr = _pool.malloc();
                new_object = new (ptr) T(std::move(*object));
                _extra.push_back(new_object);

                // Increase partition position
                _partition++;
            }
            else
            {
                // Alloc in pool and move current partition there
                void* ptr = _pool.malloc();
                T* replacement = new (ptr) T();
                _extra.push_back(replacement);
                *replacement = std::move(*partition);

                // Now alloc the object in _partition and move it
                *partition = std::move(*object);
                new_object = partition;
                _partition++;
            }

            if (_partition > _end)
            {
                // TODO(gpascualg): This is too hacky
                _partition = (T*)_extra.data();
            }
        }
    }

    return new_object;
}

template <typename T, typename B, uint16_t InitialSize, typename Track>
void pooled_static_vector<T, B, InitialSize, Track>::free_impl(T* object)
{
    // Remove from current pool
    if (is_static(object))
    {
        T* replacement = nullptr;

        if (!_extra.empty())
        {
            replacement = _extra.back();
            *object = std::move(*replacement);

            _extra.pop_back();
            _pool.free(replacement);
        }
        else 
        {
            --_current;
            if (_current != object)
            {
                replacement = _current;
                *object = std::move(*replacement);
            }
        }
    }
    else
    {
        // There should be at least one element?
        // assert_true(_pool.is_from(object), "Wrong pool!");
        // assert_true(!_extra.empty(), "Freeing from an empty pool?");

        // If there is only this one extra object
        if (_extra.size() == 1)
        {
            _pool.free(object);
            _extra.clear();
        }
        else
        {
            // Do an optimized erase by moving last to object
            T* last = _extra.back();
            _extra.pop_back();
            if (last == object)
            {
                _pool.free(object);
            }
            else
            {
                *object = std::move(*last);
                object->_ticket->_ptr = object;
                _pool.free(last);
            }
        }
    }
}


template <typename T, typename B, uint16_t InitialSize, typename Track>
void pooled_static_vector<T, B, InitialSize, Track>::free_with_partition_impl(bool p, T* object)
{
    // Remove from current pool
    if (is_static(object))
    {
        if (is_static(_partition))
        {
            // We are from the negative class, we can directly take from _extra's or static's right
            if (object >= _partition)
            {
                // We don't need to touch the partition

                if (!_extra.empty())
                {
                    T* replacement = _extra.back();
                    *object = std::move(*replacement);

                    _extra.pop_back();
                    _pool.free(replacement);
                }
                else
                {
                    --_current;
                    if (_current != object) // Also covers _current != _partition
                    {
                        *object = std::move(*_current);
                    }
                }
            }
            else
            {
                // Move partition one down
                --_partition;

                // In case we are not the partition, it must be filled
                if (object != _partition)
                {
                    // Move new partition (which is positive but in the negative side) to object's mem.
                    *object = std::move(*_partition);
                }

                // And now, fill partition
                free_with_partition_impl(p, _partition);
            }
        }
        else
        {
            // This can only happen if everything is positive, so just take whatever and place it
            assert(is_static_full() && "Static must be full to reach here");

            // Being here the object is of the positive partition, so directly reduce partition
            --_partition;
            if (_partition == (T*)_extra.data() - 1)
            {
                _partition = _end - 1;
            }

            // Either _extra.data() or nullptr if there's no _extra or it's at its end
            T* partition = partition_ptr();
            if (partition == nullptr)
            {
                // Then the partition is now the static end, which means we must
                // move said end
                --_current;
                if (_current != object)
                {
                    *object = std::move(*_current);
                }
            }
            else
            {
                assert(!_extra.empty() && "If extra storage is empty, the previous if would be true");

                // Otherwise, partition is still in extra
                *object = std::move(*partition);
                T* replacement = _extra.back();
                if (replacement != partition)
                {
                    *partition = std::move(*replacement);
                }

                _extra.pop_back();
                _pool.free(replacement);
            }
        }
    }
    else
    {
        // The object is in extra storage
        if (is_static(_partition))
        {
            // Object is on the right of the partition, thus it is negative
            // Leave the partition untouched, overwrite the value
            // If there is only this one extra object
            if (_extra.size() == 1)
            {
                _pool.free(object);
                _extra.clear();
            }
            else
            {
                // Do an optimized erase by moving last to object
                T* last = _extra.back();
                _extra.pop_back();
                if (last == object)
                {
                    _pool.free(object);
                }
                else
                {
                    *object = std::move(*last);
                    _pool.free(last);
                }
            }
        }
        else
        {
            if (p)
            {
                // Positive, which means we have to move the partition and substitute
                --_partition;
                if (_partition == (T*)_extra.data() - 1)
                {
                    _partition = _end - 1;
                }

                // Either _extra.data() or nullptr if there's no _extra or it's at its end
                T* partition = partition_ptr();
                if (partition == nullptr)
                {
                    // Nothing do do except freeing
                    _pool.free(object);
                }
                else
                {
                    T* last = _extra.back();
                    _extra.pop_back();
                    if (last == object)
                    {
                        _pool.free(object);
                    }
                    else
                    {
                        *object = std::move(*last);
                        _pool.free(last);
                    }
                }
            }
            else
            {
                // We replace it by the last
                T* last = _extra.back();
                _extra.pop_back();
                if (last == object)
                {
                    _pool.free(object);
                }
                else
                {
                    *object = std::move(*last);
                    _pool.free(last);
                }
            }
        }
    }
}

template <typename T, typename B, uint16_t InitialSize, typename Track>
T* pooled_static_vector<T, B, InitialSize, Track>::partition_change(bool p, T* object)
{
    if (p) // NEGATIVE TO POSITIVE
    {
        T* partition = partition_ptr();
        assert(partition != nullptr && "At least the object itself has to be the partition");

        if (object == partition)
        {
            ++_partition;
        }
        else
        {
            std::swap(*_partition, *object);
            object = _partition++; // Increase partition to next
        }

        if (_partition > _end)
        {
            // TODO(gpascualg): This is too hacky
            _partition = (T*)_extra.data();
        }
        return object;
    }
    else // POSITIVE TO NEGATIVE
    {
        --_partition;
        if (_partition == (T*)_extra.data() - 1)
        {
            _partition = _end - 1;
        }

        T* partition = partition_ptr();
        if (partition == nullptr)
        {
            if (is_static(object))
            {
                _partition = object;
            }
            else
            {
                _partition = (T*)_extra.data();
            }
            return object;
        }
        else if (object == partition)
        {
            return object;
        }

        std::swap(*partition, *object);
        return partition;
    }
}
