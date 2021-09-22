#pragma once

#include "common/result_of.hpp"
#include "fiber/exclusive_work_stealing.hpp"

#include <boost/fiber/buffered_channel.hpp>
#include <boost/fiber/future.hpp>
#include <function2/function2.hpp>

#include <mutex>
#include <condition_variable>


class async_executor_base
{
public:
    template <typename F>
    void submit(F&& function);

    void stop();

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

    template <typename C>
    void start(C&& callback);
};


template <uint16_t tag>
async_executor<tag>::async_executor(uint16_t number_of_threads, std::size_t capacity) :
    async_executor_base(number_of_threads, capacity)
{}

template <uint16_t tag>
template <typename C>
void async_executor<tag>::start(C&& callback)
{
    std::mutex m;
    std::condition_variable cv;
    uint8_t waiting = _number_of_threads;

    for (uint16_t i = 0; i < _number_of_threads; ++i)
    {
        _workers.emplace_back([this, callback{ std::move(callback) }, &waiting, &m, &cv]() {
            boost::fibers::use_scheduling_algorithm<exclusive_work_stealing<tag>>(_number_of_threads, true);

            callback();

            std::unique_lock<std::mutex> lk(m);
            --waiting;
            cv.notify_one();
            lk.unlock();

            worker_impl();
        });
    }

    std::unique_lock<std::mutex> lk(m);
    cv.wait(lk, [&] { return waiting == 0; });
}

template <typename F>
void async_executor_base::submit(F&& function)
{
    _channel.push(std::move(function));
}
