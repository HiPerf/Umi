#pragma once

#include "traits/shared_function.hpp"

#include <boost/fiber/mutex.hpp>
#include <boost/fiber/fiber.hpp>

#include <function2/function2.hpp>

#include <vector>


class executor_registry;

class tasks
{
public:
    using task_t = fu2::unique_function<void()>;
    using container_t = std::vector<task_t>;

public:
    tasks(executor_registry* executor) noexcept;

    template <typename T>
    void schedule(T&& task) noexcept;

    void execute() noexcept;

protected:
    void execute(container_t* buffer) noexcept;

protected:
    container_t* _current_buffer;
    container_t _tasks_buffer1;
    container_t _tasks_buffer2;
};


template <typename T>
void tasks::schedule(T&& task) noexcept
{
    _current_buffer->emplace_back(std::move(task));
}


class async_tasks : public tasks
{
public:
    using tasks::tasks;
    
    template <typename T>
    void schedule(T&& task) noexcept;
    void execute() noexcept;

private:
    boost::fibers::mutex _mutex;
};


template <typename T>
void async_tasks::schedule(T&& task) noexcept
{
    std::lock_guard<boost::fibers::mutex> lock{ _mutex };

    _current_buffer->emplace_back(std::move(task));
}
