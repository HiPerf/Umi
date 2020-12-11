#pragma once

#include "updater/updater.hpp"

#include <boost/range/adaptors.hpp>
#include <boost/range/join.hpp>


template <typename... types>
class updater_batched : public updater<updater_batched<types...>, types...>
{
    friend class updater<updater_batched<types...>, types...>;
    template <typename... D> friend class scheme;

protected:
    constexpr updater_batched(uint32_t batch_size) noexcept :
        updater<updater_batched<types...>, types...>(),
        _batch_size(batch_size)
    {}

    constexpr updater_batched(uint32_t batch_size, const tao::tuple<types...>& components) noexcept :
        updater<updater_batched<types...>, types...>(components),
        _batch_size(batch_size)
    {}

    template <typename T, typename... Args>
    constexpr void update_fiber(T* vector, Args&&... args) noexcept
    {
        int num_groups = static_cast<int>(std::ceilf(vector->size() / static_cast<float>(_batch_size)));
        auto range = vector->range();

        for (auto group = 0; group < num_groups; ++group)
        {
            boost::fibers::fiber([this, group, range, ...args{ std::forward<Args>(args) }]() mutable {
                auto it  = range.begin() + group * _batch_size;
                auto end = range.begin() + (group + 1) * _batch_size;
                
                int num_updates = 0;
                for (; it != end; ++it, ++num_updates)
                {
                    (*it)->base()->update(std::forward<Args>(args)...);
                }

                _updates_mutex.lock();
                _pending_updates -= num_updates;
                _updates_mutex.unlock();

                if (_pending_updates == 0)
                {
                    _updates_cv.notify_all();
                }
            }).detach();
        }
    }

    template <typename... vecs>
    constexpr auto clone(tao::tuple<vecs...>&& components) noexcept
    {
        return updater_batched<vecs...>(_batch_size, std::move(components));
    }

private:
    uint32_t _batch_size;
};
