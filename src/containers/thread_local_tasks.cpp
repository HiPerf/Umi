#include "containers/thread_local_tasks.hpp"
#include "updater/tasks_manager.hpp"

#include <mutex>


tasks::tasks(uint16_t max_size) noexcept :
    _max_size(max_size),
    _write_head(0),
    _end(0),
    _begin(0)
{
    _container = new task_t[max_size];
}

tasks::tasks(tasks&& other) noexcept :
    _max_size(other._max_size),
    _write_head(static_cast<uint16_t>(other._write_head)),
    _end(static_cast<uint16_t>(other._end)),
    _begin(other._end),
    _container(std::move(other._container))
{}

tasks& tasks::operator=(tasks&& other) noexcept
{
    _max_size = other._max_size;
    _write_head = static_cast<uint16_t>(other._write_head);
    _end = static_cast<uint16_t>(other._end);
    _begin = other._end;
    _container = std::move(other._container);
    return *this;
}

void tasks::execute() noexcept
{
    uint16_t current = _begin;
    
    spdlog::trace("{:x} EXEC PROG ({:d} / {:d} / {:d})", (intptr_t)(void*)this, _begin, _write_head, _end);
    for (; current != (_end % _max_size); current = (current + 1) % _max_size)
    {
        std::move(_container[current])();
    }

    _begin = current;
    spdlog::trace("{:x} EXEC DONE ({:d} / {:d} / {:d})", (intptr_t)(void*)this, _begin, _write_head, _end);
}
