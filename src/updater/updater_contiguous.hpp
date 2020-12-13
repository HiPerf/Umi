#pragma once

#include "updater/updater.hpp"

#include <boost/range/adaptors.hpp>
#include <boost/range/join.hpp>


template <typename... types>
class updater_contiguous : public updater<updater_contiguous<types...>, types...>
{
    friend class updater<updater_contiguous<types...>, types...>;
    template <typename... D> friend class scheme;

    using updater_t = updater<updater_contiguous<types...>, types...>;

protected:
    constexpr updater_contiguous() noexcept :
        updater_t()
    {}

    constexpr updater_contiguous(const tao::tuple<types...>& components) noexcept :
        updater_t(components)
    {}

    template <typename T, typename... Args>
    constexpr void update_fiber(T* vector, Args&&... args) noexcept
    {
        for (auto obj : vector->range())
        {
            obj->base()->update(std::forward<Args>(args)...);
        }

        updater_t::_updates_mutex.lock();
        updater_t::_pending_updates -= vector->size();
        updater_t::_updates_mutex.unlock();
        
        if (updater_t::_pending_updates == 0)
        {
            updater_t::_updates_cv.notify_all();
        }
    }

    template <typename... vecs>
    constexpr auto clone(tao::tuple<vecs...>&& components) noexcept
    {
        return updater_contiguous<vecs...>(std::move(components));
    }
};
