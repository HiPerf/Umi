#pragma once

#include "updater/updater.hpp"

#include <boost/range/adaptors.hpp>
#include <boost/range/join.hpp>


template <typename... types>
class updater_all_async : public updater<updater_all_async<types...>, types...>
{
    friend class updater<updater_all_async<types...>, types...>;
    template <typename... D> friend class scheme;
    
    using updater_t = updater<updater_all_async<types...>, types...>;

public:
    constexpr updater_all_async() noexcept :
        updater_t()
    {}

    constexpr updater_all_async(const tao::tuple<types...>& components) noexcept :
        updater_t(components)
    {}

protected:
    template <typename T, typename... Args>
    constexpr void update_fiber(T* vector, Args&&... args) noexcept
    {
        for (auto obj : vector->range())
        {
            boost::fibers::fiber([this, obj, ...args{ std::forward<Args>(args) }]() mutable {
                obj->base()->update(std::forward<Args>(args)...);

                updater_t::_updates_mutex.lock();
                --updater_t::_pending_updates;
                updater_t::_updates_mutex.unlock();
                
                if (updater_t::_pending_updates == 0)
                {
                    updater_t::_updates_cv.notify_all();
                }
            }).detach();
        }
    }

    template <typename... vecs>
    constexpr auto clone(tao::tuple<vecs...>&& components) noexcept
    {
        return updater_all_async<vecs...>(std::move(components));
    }
};
