#pragma once

#include "traits/shared_function.hpp"

#include <boost/fiber/mutex.hpp>
#include <boost/fiber/fiber.hpp>

#include <function2/function2.hpp>

#include <spdlog/spdlog.h>

#include <vector>


class tasks
{
public:
    using task_t = fu2::unique_function<void()>;

public:
    tasks(uint16_t max_size) noexcept;
    tasks(tasks&& other) noexcept;
    tasks& operator=(tasks&& other) noexcept;

    template <typename T>
    void schedule(T&& task) noexcept;

    void execute() noexcept;

protected:
    uint16_t _max_size;
    task_t* _container;
    std::atomic<uint16_t> _write_head;
    std::atomic<uint16_t> _end;
    uint16_t _begin;
};


template <typename T>
void tasks::schedule(T&& task) noexcept
{
    // Update write head
    uint16_t current = _write_head++;
    uint16_t expected = current + 1;
    _write_head.compare_exchange_strong(expected, expected % _max_size);
    
    // TODO(gpascualg): Assert we have not reached max queued tasks (ie. _write_head != _begin)

    // Write task
#if defined(UMI_ENABLE_DEBUG_EXTRA_LOGS)
    spdlog::trace("{:x} WRITE PROG AT {:d} ({:d} / {:d} / {:d})", (intptr_t)(void*)this, current, _begin, _write_head, _end);
#endif
    _container[current % _max_size] = std::move(task);

    // Update read head
    current = _end++;
    expected = current + 1;
    _end.compare_exchange_strong(expected, expected % _max_size);
    
#if defined(UMI_ENABLE_DEBUG_EXTRA_LOGS)
    spdlog::trace("{:x} WRITE DONE AT {:d} ({:d} / {:d} / {:d})", (intptr_t)(void*)this, current, _begin, _write_head, _end);
#endif
}
