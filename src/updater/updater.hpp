#pragma once

#include "common/tao.hpp"
#include "fiber/exclusive_work_stealing.hpp"
#include "traits/tuple.hpp"

#include <boost/fiber/fiber.hpp>
#include <boost/fiber/mutex.hpp>
#include <boost/fiber/condition_variable.hpp>

#include <tao/tuple/tuple.hpp>

#include <variant>
#include <vector>



template <typename D, typename... types>
class updater
{
protected:
    constexpr updater() noexcept :
        _pending_updates(0)
    {}

    constexpr updater(const tao::tuple<types...>& components) noexcept :
        _pending_updates(0),
        _vectors(components)
    {}

public:
    updater(const updater&) = delete;
    constexpr updater(updater&&) noexcept = default;

    template <typename... Args>
    constexpr void update(Args&&... args) noexcept
    {
        _pending_updates = 0;

        tao::apply([this, ...args{ std::forward<Args>(args) }](auto&&... vecs) mutable {
            (update_impl(vecs, std::forward<Args>(args)...), ...);
        }, _vectors);
    }

    void wait_update() noexcept
    {
        _updates_mutex.lock();
        _updates_cv.wait(_updates_mutex, [this]() { return _pending_updates == 0; });
        _updates_mutex.unlock();
    }

    template <typename... Args>
    constexpr void sync(Args&&... args) noexcept
    {
        tao::apply([this, ...args{ std::forward<Args>(args) }](auto&&... vecs) mutable {
            (sync_impl(vecs, std::forward<Args>(args)...), ...);
        }, _vectors);
    }

    template <typename T>
    constexpr auto register_vector(T* vector) noexcept
    {
        return static_cast<D&>(*this).clone(tao::tuple_cat(_vectors, tao::tuple(vector)));
    }

    template <typename T>
    constexpr auto unregister_vector(T* vector) noexcept
    {
        return static_cast<D&>(*this).clone(remove_nth<index_of<T, decltype(_vectors)>>(_vectors));
    }

protected:
    template <typename T, typename... Args>
    constexpr void update_impl(T* vector, Args&&... args) noexcept
    {
        using E = typename std::remove_pointer<std::decay_t<decltype(vector)>>::type;

        if constexpr (!E::derived_t::template has_update<std::decay_t<Args>...>())
        {
            return;
        }
        else if (vector->size())
        {
            _updates_mutex.lock();
            _pending_updates += vector->size();
            _updates_mutex.unlock();

            boost::fibers::fiber([this, vector, ...args{ std::forward<Args>(args) }]() mutable {
                static_cast<D&>(*this).update_fiber(vector, std::forward<std::decay_t<Args>>(args)...);
            }).detach();
        }
    }

    template <typename T, typename... Args>
    constexpr void sync_impl(T* vector, Args&&... args) noexcept
    {
        using E = typename std::remove_pointer<std::decay_t<decltype(vector)>>::type;

        if constexpr (!E::derived_t::template has_sync<Args...>())
        {
            return;
        }
        else
        {
            for (auto obj : vector->range())
            {
                obj->base()->base_sync(std::forward<std::decay_t<Args>>(args)...);
            }

#if !defined(NDEBUG)
            vector->unlock_writes();
#endif
        }
    }

protected:
    bool _contiguous_component_execution;
    tao::tuple<types...> _vectors;
    uint64_t _pending_updates;
    boost::fibers::mutex _updates_mutex;
    boost::fibers::condition_variable_any _updates_cv;
};
