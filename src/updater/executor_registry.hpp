#pragma once

#include <boost/fiber/mutex.hpp>
#include <boost/fiber/fiber.hpp>

#include <array>
#include <vector>


class tasks;
class async_tasks;

class executor_registry
{
    friend class tasks;
    friend class async_tasks;

public:
    static inline std::array<executor_registry*, 128> instances;

private:
    static inline executor_registry** _current = &instances[0];

public:
    static inline executor_registry* last() noexcept
    {
        return instances[0];
    }

    static inline executor_registry* current() noexcept
    {
        return *_current;
    }

protected:
    inline void push_instance() noexcept
    {
        ++_current;
        *_current = this;
    }

    inline void pop_instance() noexcept
    {
        --_current;
    }

private:
    inline void register_tasks(tasks* ts) noexcept
    {
        _mutex.lock();
        _tasks.push_back(ts);
        _mutex.unlock();
    }

    inline void register_tasks(async_tasks* ts) noexcept
    {
        _mutex.lock();
        _async_tasks.push_back(ts);
        _mutex.unlock();
    }

protected:
    // Tasks registry
    boost::fibers::mutex _mutex;
    std::vector<tasks*> _tasks;
    std::vector<async_tasks*> _async_tasks;
};