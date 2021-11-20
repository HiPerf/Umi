#pragma once

#include <boost/fiber/algo/algorithm.hpp>
#include <boost/fiber/algo/work_stealing.hpp>

#include <condition_variable>
#include <mutex>


class fiber_hash_prop : public boost::fibers::fiber_properties {
public:
    fiber_hash_prop(boost::fibers::context* ctx) :
        fiber_properties(ctx),
        _hash(2166136261),
        _name("Unnamed fiber")
    {}

    void with_name(std::string_view name)
    {
        _name = std::string(name);
        _hash = 2166136261;
        for (auto c : name)
        {
            _hash = _hash ^ c;
            _hash = _hash * 16777619;
        }
    }

    uint32_t hash()
    {
        return _hash;
    }

    const char* name()
    {
        return _name.c_str();
    }

private:
    uint32_t _hash;
    std::string _name;
};

#ifndef NDEBUG
    using exclusive_work_stealing_base_class = boost::fibers::algo::algorithm_with_properties<fiber_hash_prop>;
#else
    using exclusive_work_stealing_base_class = boost::fibers::algo::algorithm;
#endif

template <int SLOT>
class BOOST_FIBERS_DECL exclusive_work_stealing : public exclusive_work_stealing_base_class {
private:
    static std::atomic< std::uint32_t >                     counter_;
    static std::vector< boost::intrusive_ptr< exclusive_work_stealing > >    schedulers_;

    std::uint32_t                                           id_;
    std::uint32_t                                           thread_count_;
    boost::fibers::detail::context_spinlock_queue           rqueue_{ 4096 };
    boost::fibers::detail::context_spinlock_queue           bundles_{ 4096 };

    std::vector<boost::fibers::context*>                    active_bundle_;
    std::vector<boost::fibers::context*>                    bundle_building_;

    std::mutex                                              mtx_{};
    std::condition_variable                                 cnd_{};
    bool                                                    flag_{ false };
    bool                                                    suspend_;
    bool                                                    bundle_{ false };

    static void init_(std::uint32_t, std::vector< boost::intrusive_ptr< exclusive_work_stealing > >&);

public:
    exclusive_work_stealing(std::uint32_t, bool = false);

    exclusive_work_stealing(exclusive_work_stealing const&) = delete;
    exclusive_work_stealing(exclusive_work_stealing&&) = delete;

    exclusive_work_stealing& operator=(exclusive_work_stealing const&) = delete;
    exclusive_work_stealing& operator=(exclusive_work_stealing&&) = delete;

#ifndef NDEBUG
    void awakened(boost::fibers::context* ctx, fiber_hash_prop& props) noexcept override;
#else
    void awakened(boost::fibers::context*) noexcept override;
#endif

    boost::fibers::context* pick_next() noexcept override;

    void start_bundle();
    void end_bundle();

    virtual boost::fibers::context* steal() noexcept {
        return rqueue_.steal();
    }

    bool has_ready_fibers() const noexcept override {
        return !rqueue_.empty();
    }

    void suspend_until(std::chrono::steady_clock::time_point const&) noexcept override;

    void notify() noexcept override;

    static void reset() noexcept;
};

inline boost::fibers::algo::algorithm::ptr_t& get_scheduling_algorithm()
{
    thread_local boost::fibers::algo::algorithm::ptr_t algo{ nullptr };
    return algo;
}

template< typename SchedAlgo, typename ... Args >
void public_scheduling_algorithm(Args&& ... args) noexcept {
    get_scheduling_algorithm() = new SchedAlgo(std::forward< Args >(args) ...);
    boost::fibers::context::active()->get_scheduler()
        ->set_algo(get_scheduling_algorithm());
}

#include "fiber/exclusive_work_stealing_impl.hpp"
