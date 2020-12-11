#pragma once

#include "updater/updater.hpp"

#include <boost/range/adaptors.hpp>
#include <boost/range/join.hpp>


template <typename... types>
class updater_all_async : public updater<updater_all_async<types...>, types...>
{
    friend class updater<updater_all_async<types...>, types...>;
    template <typename... D> friend class scheme;

protected:
    constexpr updater_all_async() noexcept :
        updater<updater_all_async<types...>, types...>()
    {}

    constexpr updater_all_async(const tao::tuple<types...>& components) noexcept :
        updater<updater_all_async<types...>, types...>(components)
    {}

    template <typename T, typename... Args>
    constexpr void update_fiber(T* vector, Args&&... args) noexcept
    {
        for (auto obj : vector->range())
        {
            boost::fibers::fiber([this, obj, ...args{ std::forward<Args>(args) }]() mutable {
                obj->base()->update(std::forward<Args>(args)...);

                _updates_mutex.lock();
                _pending_updates -= num_updates;
                _updates_mutex.unlock();
            }).detach();
        }
    }

    template <typename... vecs>
    constexpr auto clone(tao::tuple<vecs...>&& components) noexcept
    {
        return updater_all_async<vecs...>(std::move(components));
    }
};
