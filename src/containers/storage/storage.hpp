#pragma once

#include "containers/pool_item.hpp"

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



struct scheme_view
{
    template <template <typename...> class S, typename C, typename... types>
    inline constexpr void operator()(S<types...>& scheme, C&& callback)
    {
        for (auto combined : ::ranges::views::zip(scheme.template get<types>().range()...))
        {
            std::apply(callback, combined);
        }
    }

    template <template <typename...> class S, typename C, typename... types>
    inline constexpr void continuous(S<types...>& scheme, C&& callback)
    {
        _pending_updates = scheme.size();

        boost::fibers::fiber([this, &scheme, callback = std::move(callback)]() mutable
            {
                for (auto combined : ::ranges::views::zip(scheme.template get<types>().range()...))
                {
                    std::apply(callback, combined);
                    --_pending_updates;
                }
            }).join();
    }

    template <template <typename...> class S, typename C, typename... types>
    inline constexpr void parallel(S<types...>& scheme, C&& callback)
    {
        _pending_updates = scheme.size();

        boost::fibers::fiber([this, &scheme, callback = std::move(callback)]() mutable
            {
                for (auto combined : ::ranges::views::zip(scheme.template get<types>().range()...))
                {
                    // TODO(gpascualg): Is it safe to get a reference to combined here?
                    boost::fibers::fiber([this, combined, callback = std::move(callback)]() mutable
                        {
                            std::apply(callback, combined);

                            // TODO(gpascualg): Make _pending_updates atomic and benchmark performance
                            _updates_mutex.lock();
                            --_pending_updates;
                            _updates_mutex.unlock();

                            if (_pending_updates == 0)
                            {
                                _updates_cv.notify_all();
                            }
                        }).detach();
                }

                // Wait for updates to end
                _updates_mutex.lock();
                _updates_cv.wait(_updates_mutex, [this]() { return _pending_updates == 0; });
                _updates_mutex.unlock();
            }).join();
    }

private:
    uint64_t _pending_updates;
    boost::fibers::mutex _updates_mutex;
    boost::fibers::condition_variable_any _updates_cv;
};
