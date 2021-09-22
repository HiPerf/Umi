#pragma once

#include "containers/ticket.hpp"
#include "containers/thread_local_tasks.hpp"

#include <boost/fiber/mutex.hpp>
#include <boost/fiber/fiber.hpp>

#include <array>
#include <atomic>
#include <vector>


class tasks;

class tasks_manager
{
    friend class tasks;

public:
    tasks_manager() = default;

    tasks_manager(tasks_manager&& other) noexcept :
        _tasks(std::move(other._tasks))
    {}

    tasks_manager& operator=(tasks_manager&& rhs) noexcept
    {
        _tasks = std::move(rhs._tasks);
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
        thread_local tasks ts(this, 2048);
        return ts;
    }

protected:
    void execute_tasks() noexcept
    {
        for (auto ts : tasks_manager::_tasks)
        {
            ts->execute();
        }
    }

private:
    inline void register_tasks(tasks* ts) noexcept
    {
        _mutex.lock();
        _tasks.push_back(ts);
        _mutex.unlock();
    }

protected:
    boost::fibers::mutex _mutex;
    std::vector<tasks*> _tasks;
};
