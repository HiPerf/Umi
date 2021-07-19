#include "async/async_executor.hpp"


async_executor_base::async_executor_base(uint16_t number_of_threads, std::size_t capacity) :
    _number_of_threads(number_of_threads),
    _channel(capacity)
{}


void async_executor_base::worker_impl()
{
    auto task_function = typename decltype(_channel)::value_type{};
    while (boost::fibers::channel_op_status::success == _channel.pop(task_function))
    {
        boost::fibers::fiber([task = std::move(task_function)]() mutable
        {
            std::move(task)();
        }).detach();
    }
}

void async_executor_base::stop()
{
    _channel.close();
    for (auto& t : _workers)
    {
        t.join();
    }
}
