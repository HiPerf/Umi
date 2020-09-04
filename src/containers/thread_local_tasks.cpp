#include "containers/thread_local_tasks.hpp"

#include <mutex>


tasks::tasks()
{
    _current_buffer = &_tasks_buffer1;
    thread_local_storage<tasks>::store(this);
}

void tasks::execute()
{
    auto old = _current_buffer;
    auto other = _current_buffer == &_tasks_buffer1 ? &_tasks_buffer2 : &_tasks_buffer1;
    execute(other);
    _current_buffer = other;
    execute(old);
}

void tasks::execute(container_t* buffer)
{
    for (auto it = buffer->begin(); it != buffer->end(); ++it)
    {
        std::move(*it)();
    }

    buffer->clear();
}

void async_tasks::execute()
{
    auto old = _current_buffer;
    auto other = _current_buffer == &_tasks_buffer1 ? &_tasks_buffer2 : &_tasks_buffer1;
    tasks::execute(other);

    _mutex.lock();
    _current_buffer = other;
    _mutex.unlock();

    tasks::execute(old);
}
