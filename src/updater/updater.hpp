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



template <typename... types>
class updater
{
public:
    constexpr updater(bool contiguous_component_execution) noexcept :
        _contiguous_component_execution(contiguous_component_execution),
        _pending_updates(0)
    {}

    constexpr updater(bool contiguous_component_execution, const tao::tuple<types...>& components) noexcept :
        _contiguous_component_execution(contiguous_component_execution),
        _pending_updates(0),
        _vectors(components)
    {}

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
    constexpr updater<T*, types...> register_vector(T* vector) noexcept
    {
        return updater(_contiguous_component_execution, tao::tuple_cat(_vectors, tao::tuple(vector)));
    }

    template <typename T>
    constexpr auto unregister_vector(T* vector) noexcept
    {
        return updater(_contiguous_component_execution, remove_nth<index_of<T, decltype(_vectors)>>(_vectors));
    }

private:
    template <typename T, typename... Args>
    constexpr void update_impl(T* vector, Args&&... args) noexcept
    {
        using E = typename std::remove_pointer<std::decay_t<decltype(vector)>>::type;

        if constexpr (!E::derived_t::template has_update<std::decay_t<Args>...>())
        {
            return;
        }
        else
        {
            _updates_mutex.lock();
            _pending_updates += vector->size();
            _updates_mutex.unlock();

            boost::fibers::fiber([this, vector, ...args{ std::forward<Args>(args) }]() mutable {
                update_fiber(vector, std::forward<std::decay_t<Args>>(args)...);
            }).detach();
        }
    }

    template <typename T, typename... Args>
    constexpr void update_fiber(T* vector, Args&&... args) noexcept
    {
        if (_contiguous_component_execution)
        {
            reinterpret_cast<exclusive_work_stealing<0>*>(get_scheduling_algorithm().get())->start_bundle();
        }

        for (auto obj : vector->range())
        {
            boost::fibers::fiber([this, obj, ...args{ std::forward<Args>(args) }]() mutable {
                obj->base()->update(std::forward<Args>(args)...);
                
                _updates_mutex.lock();
                --_pending_updates;
                _updates_mutex.unlock();

                if (_pending_updates == 0)
                {
                    _updates_cv.notify_all();
                }
            }).detach();
        }

        if (_contiguous_component_execution)
        {
            reinterpret_cast<exclusive_work_stealing<0>*>(get_scheduling_algorithm().get())->end_bundle();
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
                obj->base()->sync(std::forward<std::decay_t<Args>>(args)...);
            }
        }
    }

private:
    bool _contiguous_component_execution;
    tao::tuple<types...> _vectors;
    uint64_t _pending_updates;
    boost::fibers::mutex _updates_mutex;
    boost::fibers::condition_variable_any _updates_cv;
};
