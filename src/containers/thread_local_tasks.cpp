#include "containers/thread_local_tasks.hpp"
#include "updater/executor_registry.hpp"

#include <mutex>


tasks::tasks(executor_registry* executor, uint16_t max_size) noexcept :
    _max_size(max_size),
    _write_head(0),
    _end(0),
    _begin(0)
{
    _container = new task_t[max_size];
    executor->register_tasks(this);
}

void tasks::execute() noexcept
{
    uint16_t current = _begin;

    for (; current != (_end % _max_size); current = (current + 1) % _max_size)
    {
        std::move(_container[current])();
    }

    _begin = current;
}
