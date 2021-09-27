#pragma once

#include "containers/ticket.hpp"
#include "containers/thread_local_tasks.hpp"

#include <boost/fiber/mutex.hpp>
#include <boost/fiber/fiber.hpp>

#include <array>
#include <atomic>


class tasks;


namespace detail
{
    template<size_t...Is>
    std::array<tasks, sizeof...(Is)> make_tasks(uint16_t max_size, std::index_sequence<Is...>) {
        return { ((void)Is, tasks(max_size))... };
    }
}

template <uint16_t max_threads, typename tag>
class tasks_manager
{
public:
    tasks_manager(uint16_t max_size) :
        _tasks(detail::make_tasks(max_size, std::make_index_sequence<max_threads>()))
    {}

    tasks_manager(tasks_manager&& other) noexcept :
        _tasks(std::move(other._tasks))
    {}

    tasks_manager& operator=(tasks_manager&& rhs) noexcept
    {
        _tasks = std::move(rhs._tasks);
        return *this;
    }

    template <typename C>
    constexpr void schedule(C&& callback) noexcept
    {
        get_scheduler().schedule(fu2::unique_function<void()>(std::move(callback))); // ;
    }

    template <typename C, typename... Args>
    constexpr void schedule_if(C&& callback, Args&&... tickets) noexcept
    {
        schedule([callback = std::move(callback), tickets...]() mutable {
            if ((tickets->valid() && ...))
            {
                callback(tickets->get()->derived()...);
            }
        });
    }

    inline tasks& get_scheduler() noexcept
    {
        thread_local uint16_t index = get_count()++;
        return _tasks[index];
    }

protected:
    inline std::atomic<uint16_t>& get_count() noexcept
    {
        static std::atomic<uint16_t> current = 0;
        return current;
    }

    void execute_tasks() noexcept
    {
        for (uint16_t i = 0; i < get_count(); ++i)
        {
            _tasks[i].execute();
        }
    }

protected:
    std::array<tasks, max_threads> _tasks;
};
