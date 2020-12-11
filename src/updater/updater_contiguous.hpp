#pragma once

#include "updater/updater.hpp"

#include <boost/range/adaptors.hpp>
#include <boost/range/join.hpp>


template <typename... types>
class updater_contiguous : public updater<updater_contiguous<types...>, types...>
{
    friend class updater<updater_contiguous<types...>, types...>;
    template <typename... D> friend class scheme;

protected:
    constexpr updater_contiguous() noexcept :
        updater<updater_contiguous<types...>, types...>()
    {}

    constexpr updater_contiguous(const tao::tuple<types...>& components) noexcept :
        updater<updater_contiguous<types...>, types...>(components)
    {}

    template <typename T, typename... Args>
    constexpr void update_fiber(T* vector, Args&&... args) noexcept
    {
        for (auto obj : vector->range())
        {
            obj->base()->update(std::forward<Args>(args)...);
        }

        _updates_mutex.lock();
        _pending_updates -= num_updates;
        _updates_mutex.unlock();
    }

    template <typename... vecs>
    constexpr auto clone(tao::tuple<vecs...>&& components) noexcept
    {
        return updater_contiguous<vecs...>(std::move(components));
    }
};
