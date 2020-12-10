#pragma once

#include "common/result_of.hpp"
#include "fiber/exclusive_work_stealing.hpp"

#include <boost/fiber/buffered_channel.hpp>
#include <boost/fiber/future.hpp>
#include <function2/function2.hpp>



class async_executor_base
{
public:
    template <typename F>
    void submit(F&& function);

protected:
    async_executor_base(uint16_t number_of_threads, std::size_t capacity);
    void worker_impl();

protected:
    std::vector<std::thread> _workers;
    boost::fibers::buffered_channel<fu2::unique_function<void()>> _channel;
    uint16_t _number_of_threads;
};

template <uint16_t tag>
class async_executor : public async_executor_base
{
public:
    async_executor(uint16_t number_of_threads, std::size_t capacity);

    void worker();
};


template <uint16_t tag>
async_executor<tag>::async_executor(uint16_t number_of_threads, std::size_t capacity) :
    async_executor_base(number_of_threads, capacity)
{
    for (uint16_t i = 0; i < number_of_threads; ++i)
    {
        _workers.emplace_back(&async_executor<tag>::worker, this);
    }
}

template <uint16_t tag>
void async_executor<tag>::worker()
{
    boost::fibers::use_scheduling_algorithm<exclusive_work_stealing<tag>>(_number_of_threads, true);

    worker_impl();
}

template <typename F>
void async_executor_base::submit(F&& function)
{
    _channel.push(std::move(function));
}
