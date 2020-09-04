#pragma once

#include "traits/shared_function.hpp"

#include <boost/fiber/mutex.hpp>
#include <boost/fiber/fiber.hpp>

#include <function2/function2.hpp>

#include <vector>

template <typename T>
class thread_local_storage
{
public:
    using container_t = std::vector<T*>;

public:
    static inline void store(T* container)
    {
        _mutex.lock();
        _containers.push_back(container);
        _mutex.unlock();
    }

    static inline container_t& get()
    {
        return _containers;
    }

private:
    static inline boost::fibers::mutex _mutex;
    static inline container_t _containers;
};


class tasks
{
public:
    using task_t = fu2::unique_function<void()>;
    using container_t = std::vector<task_t>;

public:
    tasks();

    template <typename T>
    void schedule(T&& task);

    void execute();

protected:
    void execute(container_t* buffer);

protected:
    container_t* _current_buffer;
    container_t _tasks_buffer1;
    container_t _tasks_buffer2;
};


template <typename T>
void tasks::schedule(T&& task)
{
    _current_buffer->emplace_back(std::move(task));
}


class async_tasks : public tasks
{
public:
    using tasks::tasks;

    template <typename T>
    void schedule(T&& task);
    void execute();

private:
    boost::fibers::mutex _mutex;
};


template <typename T>
void async_tasks::schedule(T&& task)
{
    std::lock_guard<boost::fibers::mutex> lock{ _mutex };

    _current_buffer->emplace_back(std::move(task));
}
