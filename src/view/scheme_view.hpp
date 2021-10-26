#pragma once

#include "storage/storage.hpp"

#include <boost/config.hpp>
#include <boost/fiber/barrier.hpp>
#include <boost/fiber/condition_variable.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/mutex.hpp>
#include <boost/range.hpp>
#include <boost/range/combine.hpp>

#include <range/v3/view/zip.hpp>
#include <tao/tuple/tuple.hpp>

#include <tuple>
#include <type_traits>
#include <vector>


struct multi_waitable
{
    friend struct scheme_view;
    template <typename... components> friend struct partial_scheme_view;
    friend struct scheme_view_until_partition;
    template <typename... components> friend struct partial_scheme_view_until_partition;
    friend struct scheme_view_from_partition;
    template <typename... components> friend struct partial_scheme_view_from_partition;

public:
    using barrier_t = const std::shared_ptr<boost::fibers::barrier>&;

    multi_waitable() noexcept = default;

    multi_waitable(multi_waitable && other) noexcept :
        _barriers(std::move(other._barriers))
    {
        assert(other.done() && "Cannot move while updates are pending");
    }

    multi_waitable& operator=(multi_waitable && rhs) noexcept
    {
        assert(rhs.done() && "Cannot move while updates are pending");
        _barriers = std::move(rhs._barriers);
        return *this;
    }

    inline void wait() noexcept
    {
        // No need to join if there is nothing to wait for
        assert(!_barriers.empty() && "Can't wait if there are no barriers");

        // Elements at the front are more likely to be done
        _barriers.back()->wait();
        _barriers.pop_back();
    }

    inline bool done() const noexcept
    {
        return _barriers.empty();
    }

protected:
    const std::shared_ptr<boost::fibers::barrier>& new_waitable(uint16_t size)
    {
        return _barriers.emplace_back(std::make_shared<boost::fibers::barrier>(size + 1));
    }

private:
    std::vector<std::shared_ptr<boost::fibers::barrier>> _barriers;
};

class reusable_barrier {
private:
    std::size_t initial_;
    std::size_t current_;
    std::size_t cycle_{ 0 };
    boost::fibers::mutex mtx_{};
    boost::fibers::condition_variable cond_{};

public:
    reusable_barrier() :
        initial_{ 0 },
        current_{ 0 }
    {}

    explicit reusable_barrier(std::size_t initial) :
        initial_{ initial },
        current_{ initial_ } 
    {}

    reusable_barrier(reusable_barrier const&) = delete;
    reusable_barrier& operator=(reusable_barrier const&) = delete;

    bool wait() {
        std::unique_lock< boost::fibers::mutex > lk{ mtx_ };
        const std::size_t cycle = cycle_;
        if (0 == --current_) {
            ++cycle_;
            current_ = initial_;
            lk.unlock(); // no pessimization
            cond_.notify_all();
            return true;
        }

        cond_.wait(lk, [&] { return cycle != cycle_; });
        return false;
    }

    void reset(std::size_t initial)
    {
        initial_ = initial;
        current_ = initial;
    }
};

struct single_waitable
{
    friend struct scheme_view;
    template <typename... components> friend struct partial_scheme_view;
    friend struct scheme_view_until_partition;
    template <typename... components> friend struct partial_scheme_view_until_partition;
    friend struct scheme_view_from_partition;
    template <typename... components> friend struct partial_scheme_view_from_partition;

public:
    using barrier_t = reusable_barrier*;

    single_waitable() noexcept = default;

    single_waitable(single_waitable && other) noexcept :
        _barrier(),
        _done(other._done)
    {
        assert(other.done() && "Cannot move while updates are pending");
    }

    single_waitable& operator=(single_waitable&& rhs) noexcept
    {
        assert(rhs.done() && "Cannot move while updates are pending");
        _done = std::move(rhs._done);
        return *this;
    }

    inline void wait() noexcept
    {
        // Elements at the front are more likely to be done
        _barrier.wait();
        _done = true;
    }

    inline bool done() const noexcept
    {
        return _done;
    }

protected:
    reusable_barrier* new_waitable(uint16_t size)
    {
        _done = false;
        _barrier.reset(size + 1);
        return &_barrier;
    }

private:
    reusable_barrier _barrier;
    bool _done;
};


struct scheme_view
{
    template <typename W, template <typename...> class S, typename C, typename... types>
    inline static constexpr void continuous(W& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        static_assert(
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::continuous) && ...) ||
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::partitioned) && ...),
            "Use continuous_by when the scheme contains mixed layouts"
            );

        // Create a barrier, exit if we have nothing to do
        typename W::barrier_t barrier = waitable.new_waitable(int(scheme.size() > 0));
        if (scheme.size() == 0)
        {
            return;
        }

        boost::fibers::fiber([barrier, &scheme, callback = std::move(callback)]() mutable
        {
            auto zip = ::ranges::views::zip(scheme.template get<types>().range()...);
            for (auto combined : zip)
            {
                std::apply(callback, combined);
            }

#if !defined(NDEBUG)
            (..., scheme.template get<types>().unlock_writes());
#endif

            barrier->wait();
        }).detach();
    }

    template <typename W, typename By, template <typename...> class S, typename C, typename... types>
    inline static constexpr void continuous_by(W& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        // Create a barrier, exit if we have nothing to do
        typename W::barrier_t barrier = waitable.new_waitable(int(scheme.size() > 0));
        if (scheme.size() == 0)
        {
            return;
        }

        boost::fibers::fiber([barrier, &scheme, callback = std::move(callback)]() mutable
        {
            auto& component = scheme.template get<By>();
            for (auto obj : component.range())
            {
                std::apply(callback, scheme.search(obj->id()));
            }

#if !defined(NDEBUG)
            (..., scheme.template get<types>().unlock_writes());
#endif

            barrier->wait();
        }).detach();
    }

    template <typename W, template <typename...> class S, typename C, typename... types>
    inline static constexpr void parallel(W& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        static_assert(
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::continuous) && ...) ||
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::partitioned) && ...),
            "Use parallel_by when the scheme contains mixed layouts"
            );

        // Create a barrier, exit if we have nothing to 
#if !defined(NDEBUG)
        if (scheme.size() == 0)
        {
            typename W::barrier_t barrier = waitable.new_waitable(0);
            return;
        }
        typename W::barrier_t barrier = waitable.new_waitable(scheme.size() + 1);
#else
        typename W::barrier_t barrier = waitable.new_waitable(scheme.size());
        if (scheme.size() == 0)
        {
            return;
    }
#endif

        // TODO(gpascualg): Do we need this outter fiber?
        boost::fibers::fiber([barrier, &scheme, callback = std::move(callback)]() mutable
        {
            auto zip = ::ranges::views::zip(scheme.template get<types>().range()...);
            for (auto combined : zip)
            {
                // TODO(gpascualg): Is it safe to get a reference to combined here?
                boost::fibers::fiber([barrier, combined, callback = std::move(callback)]() mutable
                {
                    std::apply(callback, combined);
                    barrier->wait();
                }).detach();
            }

#if !defined(NDEBUG)
            barrier->wait();
            (..., scheme.template get<types>().unlock_writes());
#endif
        }).detach();
}

    template <typename W, typename By, template <typename...> class S, typename O, typename C, typename... types>
    inline static constexpr void parallel_by(W& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        // Create a barrier, exit if we have nothing to do
#if !defined(NDEBUG)
        if (scheme.size() == 0)
        {
            typename W::barrier_t barrier = waitable.new_waitable(0);
            return;
        }
        typename W::barrier_t barrier = waitable.new_waitable(scheme.size() + 1);
#else
        typename W::barrier_t barrier = waitable.new_waitable(scheme.size());
        if (scheme.size() == 0)
        {
            return;
        }
#endif

        boost::fibers::fiber([barrier, &scheme, callback = std::move(callback)]() mutable
        {
            auto& component = scheme.template get<By>();
            for (auto obj : component.range())
            {
                boost::fibers::fiber([barrier, &scheme, id = obj->id(), callback = std::move(callback)]() mutable
                {
                    std::apply(callback, scheme.search(id));
                    barrier->wait();
                }).detach();
            }

#if !defined(NDEBUG)
            barrier->wait();
            (..., scheme.template get<types>().unlock_writes());
#endif
        }).detach();
    }

private:
    scheme_view();
};

template <typename... components>
struct partial_scheme_view
{
    template <typename W, template <typename...> class S, typename C, typename... types>
    inline static constexpr void continuous(W& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        static_assert(
            (has_storage_tag(S<types...>::template orchestrator_t<components>::tag, storage_grow::none, storage_layout::continuous) && ...) ||
            (has_storage_tag(S<types...>::template orchestrator_t<components>::tag, storage_grow::none, storage_layout::partitioned) && ...),
            "Use continuous_by when the scheme contains mixed layouts"
            );

        // Create a barrier, exit if we have nothing to do
        typename W::barrier_t barrier = waitable.new_waitable(int(scheme.size() > 0));
        if (scheme.size() == 0)
        {
            return;
        }

        boost::fibers::fiber([barrier, &scheme, callback = std::move(callback)]() mutable
        {
            auto zip = ::ranges::views::zip(scheme.template get<components>().range()...);
            for (auto combined : zip)
            {
                std::apply(callback, combined);
            }

#if !defined(NDEBUG)
            (..., scheme.template get<components>().unlock_writes());
#endif

            barrier->wait();
        }).detach();
    }

    template <typename W, typename By, template <typename...> class S, typename C, typename... types>
    inline static constexpr void continuous_by(W& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        // Create a barrier, exit if we have nothing to do
        typename W::barrier_t barrier = waitable.new_waitable(int(scheme.size() > 0));
        if (scheme.size() == 0)
        {
            return;
        }

        boost::fibers::fiber([barrier, &scheme, callback = std::move(callback)]() mutable
        {
            auto& component = scheme.template get<By>();
            for (auto obj : component.range())
            {
                std::apply(callback, scheme.search(obj->id()));
            }

#if !defined(NDEBUG)
            (..., scheme.template get<components>().unlock_writes());
#endif

            barrier->wait();
        }).detach();
    }

    template <typename W, template <typename...> class S, typename C, typename... types>
    inline static constexpr void parallel(W& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        static_assert(
            (has_storage_tag(S<types...>::template orchestrator_t<components>::tag, storage_grow::none, storage_layout::continuous) && ...) ||
            (has_storage_tag(S<types...>::template orchestrator_t<components>::tag, storage_grow::none, storage_layout::partitioned) && ...),
            "Use parallel_by when the scheme contains mixed layouts"
            );

        // Create a barrier, exit if we have nothing to do
#if !defined(NDEBUG)
        if (scheme.size() == 0)
        {
            typename W::barrier_t barrier = waitable.new_waitable(0);
            return;
        }
        typename W::barrier_t barrier = waitable.new_waitable(scheme.size() + 1);
#else
        typename W::barrier_t barrier = waitable.new_waitable(scheme.size());
        if (scheme.size() == 0)
        {
            return;
        }
#endif

        // TODO(gpascualg): Do we need this outter fiber?
        boost::fibers::fiber([barrier, &scheme, callback = std::move(callback)]() mutable
        {
            auto zip = ::ranges::views::zip(scheme.template get<components>().range()...);
            for (auto combined : zip)
            {
                // TODO(gpascualg): Is it safe to get a reference to combined here?
                boost::fibers::fiber([barrier, combined, callback = std::move(callback)]() mutable
                {
                    std::apply(callback, combined);
                    barrier->wait();
                }).detach();
            }

#if !defined(NDEBUG)
            barrier->wait();
            (..., scheme.template get<components>().unlock_writes());
#endif
        }).detach();
    }

    template <typename W, typename By, template <typename...> class S, typename O, typename C, typename... types>
    inline static constexpr void parallel_by(W& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        // Create a barrier, exit if we have nothing to do
#if !defined(NDEBUG)
        if (scheme.size() == 0)
        {
            typename W::barrier_t barrier = waitable.new_waitable(0);
            return;
        }
        typename W::barrier_t barrier = waitable.new_waitable(scheme.size() + 1);
#else
        typename W::barrier_t barrier = waitable.new_waitable(scheme.size());
        if (scheme.size() == 0)
        {
            return;
        }
#endif

        boost::fibers::fiber([barrier, &scheme, callback = std::move(callback)]() mutable
        {
            auto& component = scheme.template get<By>();
            for (auto obj : component.range())
            {
                boost::fibers::fiber([barrier, &scheme, id = obj->id(), callback = std::move(callback)]() mutable
                {
                    std::apply(callback, scheme.search(id));
                    barrier->wait();
                }).detach();
            }

#if !defined(NDEBUG)
            barrier->wait();
            (..., scheme.template get<components>().unlock_writes());
#endif
        }).detach();
    }

private:
    partial_scheme_view();
};

struct scheme_view_until_partition
{
    template <typename W, template <typename...> class S, typename C, typename... types>
    inline static constexpr void continuous(W& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        static_assert(
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::continuous) && ...) ||
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::partitioned) && ...),
            "Use continuous_by when the scheme contains mixed layouts"
            );

        // Create a barrier, exit if we have nothing to do
        typename W::barrier_t barrier = waitable.new_waitable(int(scheme.size_until_partition() > 0));
        if (scheme.size_until_partition() == 0)
        {
            return;
        }

        boost::fibers::fiber([barrier, &scheme, callback = std::move(callback)]() mutable
        {
            auto zip = ::ranges::views::zip(scheme.template get<types>().range_until_partition()...);
            for (auto combined : zip)
            {
                std::apply(callback, combined);
            }

#if !defined(NDEBUG)
            (..., scheme.template get<types>().unlock_writes());
#endif

            barrier->wait();
        }).detach();
    }

    template <typename W, typename By, template <typename...> class S, typename C, typename... types>
    inline static constexpr void continuous_by(W& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        // Create a barrier, exit if we have nothing to do
        typename W::barrier_t barrier = waitable.new_waitable(int(scheme.size_until_partition() > 0));
        if (scheme.size_until_partition() == 0)
        {
            return;
        }

        boost::fibers::fiber([barrier, &scheme, callback = std::move(callback)]() mutable
        {
            auto& component = scheme.template get<By>();
            for (auto obj : component.range_until_partition())
            {
                std::apply(callback, scheme.search(obj->id()));
            }

#if !defined(NDEBUG)
            (..., scheme.template get<types>().unlock_writes());
#endif

            barrier->wait();
        }).detach();
    }

    template <typename W, template <typename...> class S, typename C, typename... types>
    inline static constexpr void parallel(W& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        static_assert(
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::continuous) && ...) ||
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::partitioned) && ...),
            "Use parallel_by when the scheme contains mixed layouts"
            );

        // Create a barrier, exit if we have nothing to do
#if !defined(NDEBUG)
        if (scheme.size_until_partition() == 0)
        {
            typename W::barrier_t barrier = waitable.new_waitable(0);
            return;
        }
        typename W::barrier_t barrier = waitable.new_waitable(scheme.size_until_partition() + 1);
#else
        typename W::barrier_t barrier = waitable.new_waitable(scheme.size_until_partition());
        if (scheme.size() == 0)
        {
            return;
        }
#endif

        // TODO(gpascualg): Do we need this outter fiber?
        boost::fibers::fiber([barrier, &scheme, callback = std::move(callback)]() mutable
        {
            auto zip = ::ranges::views::zip(scheme.template get<types>().range_until_partition()...);
            for (auto combined : zip)
            {
                // TODO(gpascualg): Is it safe to get a reference to combined here?
                boost::fibers::fiber([barrier, combined, callback = std::move(callback)]() mutable
                {
                    std::apply(callback, combined);
                    barrier->wait();
                }).detach();
            }

#if !defined(NDEBUG)
            barrier->wait();
            (..., scheme.template get<types>().unlock_writes());
#endif
        }).detach();
    }

    template <typename W, typename By, template <typename...> class S, typename O, typename C, typename... types>
    inline static constexpr void parallel_by(W& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        // Create a barrier, exit if we have nothing to do
#if !defined(NDEBUG)
        if (scheme.size_until_partition() == 0)
        {
            typename W::barrier_t barrier = waitable.new_waitable(0);
            return;
        }
        typename W::barrier_t barrier = waitable.new_waitable(scheme.size_until_partition() + 1);
#else
        typename W::barrier_t barrier = waitable.new_waitable(scheme.size_until_partition());
        if (scheme.size() == 0)
        {
            return;
        }
#endif

        boost::fibers::fiber([barrier, &scheme, callback = std::move(callback)]() mutable
        {
            auto& component = scheme.template get<By>();
            for (auto obj : component.range_until_partition())
            {
                boost::fibers::fiber([barrier, &scheme, id = obj->id(), callback = std::move(callback)]() mutable
                {
                    std::apply(callback, scheme.search(id));
                    barrier->wait();
                }).detach();
            }

#if !defined(NDEBUG)
            barrier->wait();
            (..., scheme.template get<types>().unlock_writes());
#endif
        }).detach();
    }

private:
    scheme_view_until_partition();
};

template <typename... components>
struct partial_scheme_view_until_partition
{
    template <typename W, template <typename...> class S, typename C, typename... types>
    inline static constexpr void continuous(W& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        static_assert(
            (has_storage_tag(S<types...>::template orchestrator_t<components>::tag, storage_grow::none, storage_layout::continuous) && ...) ||
            (has_storage_tag(S<types...>::template orchestrator_t<components>::tag, storage_grow::none, storage_layout::partitioned) && ...),
            "Use continuous_by when the scheme contains mixed layouts"
            );

        // Create a barrier, exit if we have nothing to do
        typename W::barrier_t barrier = waitable.new_waitable(int(scheme.size_until_partition() > 0));
        if (scheme.size_until_partition() == 0)
        {
            return;
        }

        boost::fibers::fiber([barrier, &scheme, callback = std::move(callback)]() mutable
        {
            auto zip = ::ranges::views::zip(scheme.template get<components>().range_until_partition()...);
            for (auto combined : zip)
            {
                std::apply(callback, combined);
            }

#if !defined(NDEBUG)
            (..., scheme.template get<components>().unlock_writes());
#endif

            barrier->wait();
        }).detach();
    }

    template <typename W, typename By, template <typename...> class S, typename C, typename... types>
    inline static constexpr void continuous_by(W& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        // Create a barrier, exit if we have nothing to do
        typename W::barrier_t barrier = waitable.new_waitable(int(scheme.size_until_partition() > 0));
        if (scheme.size_until_partition() == 0)
        {
            return;
        }

        boost::fibers::fiber([barrier, &scheme, callback = std::move(callback)]() mutable
        {
            auto& component = scheme.template get<By>();
            for (auto obj : component.range_until_partition())
            {
                std::apply(callback, scheme.search(obj->id()));
            }

#if !defined(NDEBUG)
            (..., scheme.template get<components>().unlock_writes());
#endif

            barrier->wait();
        }).detach();
    }

    template <typename W, template <typename...> class S, typename C, typename... types>
    inline static constexpr void parallel(W& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        static_assert(
            (has_storage_tag(S<types...>::template orchestrator_t<components>::tag, storage_grow::none, storage_layout::continuous) && ...) ||
            (has_storage_tag(S<types...>::template orchestrator_t<components>::tag, storage_grow::none, storage_layout::partitioned) && ...),
            "Use parallel_by when the scheme contains mixed layouts"
            );

        // Create a barrier, exit if we have nothing to do
#if !defined(NDEBUG)
        if (scheme.size_until_partition() == 0)
        {
            typename W::barrier_t barrier = waitable.new_waitable(0);
            return;
        }
        typename W::barrier_t barrier = waitable.new_waitable(scheme.size_until_partition() + 1);
#else
        typename W::barrier_t barrier = waitable.new_waitable(scheme.size_until_partition());
        if (scheme.size() == 0)
        {
            return;
        }
#endif

        // TODO(gpascualg): Do we need this outter fiber?
        boost::fibers::fiber([barrier, &scheme, callback = std::move(callback)]() mutable
        {
            auto zip = ::ranges::views::zip(scheme.template get<components>().range_until_partition()...);
            for (auto combined : zip)
            {
                // TODO(gpascualg): Is it safe to get a reference to combined here?
                boost::fibers::fiber([barrier, combined, callback = std::move(callback)]() mutable
                {
                    std::apply(callback, combined);
                    barrier->wait();
                }).detach();
            }

#if !defined(NDEBUG)
            barrier->wait();
            (..., scheme.template get<components>().unlock_writes());
#endif
        }).detach();
    }

    template <typename W, typename By, template <typename...> class S, typename O, typename C, typename... types>
    inline static constexpr void parallel_by(W& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        // Create a barrier, exit if we have nothing to do
#if !defined(NDEBUG)
        if (scheme.size_until_partition() == 0)
        {
            typename W::barrier_t barrier = waitable.new_waitable(0);
            return;
        }
        typename W::barrier_t barrier = waitable.new_waitable(scheme.size_until_partition() + 1);
#else
        typename W::barrier_t barrier = waitable.new_waitable(scheme.size_until_partition());
        if (scheme.size() == 0)
        {
            return;
        }
#endif

        boost::fibers::fiber([barrier, &scheme, callback = std::move(callback)]() mutable
        {
            auto& component = scheme.template get<By>();
            for (auto obj : component.range_until_partition())
            {
                boost::fibers::fiber([barrier, &scheme, id = obj->id(), callback = std::move(callback)]() mutable
                {
                    std::apply(callback, scheme.search(id));
                    barrier->wait();
                }).detach();
            }

#if !defined(NDEBUG)
            barrier->wait();
            (..., scheme.template get<components>().unlock_writes());
#endif
        }).detach();
    }

private:
    partial_scheme_view_until_partition();
};

struct scheme_view_from_partition
{
    template <typename W, template <typename...> class S, typename C, typename... types>
    inline static constexpr void continuous(W& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        static_assert(
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::continuous) && ...) ||
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::partitioned) && ...),
            "Use continuous_by when the scheme contains mixed layouts"
            );

        // Create a barrier, exit if we have nothing to do
        typename W::barrier_t barrier = waitable.new_waitable(int(scheme.size_from_partition() > 0));
        if (scheme.size_from_partition() == 0)
        {
            return;
        }

        boost::fibers::fiber([barrier, &scheme, callback = std::move(callback)]() mutable
        {
            auto zip = ::ranges::views::zip(scheme.template get<types>().range_from_partition()...);
            for (auto combined : zip)
            {
                std::apply(callback, combined);
            }

#if !defined(NDEBUG)
            (..., scheme.template get<types>().unlock_writes());
#endif

            barrier->wait();
        }).detach();
    }

    template <typename W, typename By, template <typename...> class S, typename C, typename... types>
    inline static constexpr void continuous_by(W& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        // Create a barrier, exit if we have nothing to do
        typename W::barrier_t barrier = waitable.new_waitable(int(scheme.size_from_partition() > 0));
        if (scheme.size_from_partition() == 0)
        {
            return;
        }

        boost::fibers::fiber([barrier, &scheme, callback = std::move(callback)]() mutable
        {
            auto& component = scheme.template get<By>();
            for (auto obj : component.range_from_partition())
            {
                std::apply(callback, scheme.search(obj->id()));
            }

#if !defined(NDEBUG)
            (..., scheme.template get<types>().unlock_writes());
#endif

            barrier->wait();
        }).detach();
    }

    template <typename W, template <typename...> class S, typename C, typename... types>
    inline static constexpr void parallel(W& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        static_assert(
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::continuous) && ...) ||
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::partitioned) && ...),
            "Use parallel_by when the scheme contains mixed layouts"
            );

        // Create a barrier, exit if we have nothing to do
#if !defined(NDEBUG)
        if (scheme.size_from_partition() == 0)
        {
            typename W::barrier_t barrier = waitable.new_waitable(0);
            return;
        }
        typename W::barrier_t barrier = waitable.new_waitable(scheme.size_from_partition() + 1);
#else
        typename W::barrier_t barrier = waitable.new_waitable(scheme.size_from_partition());
        if (scheme.size() == 0)
        {
            return;
        }
#endif

        // TODO(gpascualg): Do we need this outter fiber?
        boost::fibers::fiber([barrier, &scheme, callback = std::move(callback)]() mutable
        {
            auto zip = ::ranges::views::zip(scheme.template get<types>().range_from_partition()...);
            for (auto combined : zip)
            {
                // TODO(gpascualg): Is it safe to get a reference to combined here?
                boost::fibers::fiber([barrier, combined, callback = std::move(callback)]() mutable
                {
                    std::apply(callback, combined);
                    barrier->wait();
                }).detach();
            }

#if !defined(NDEBUG)
            barrier->wait();
            (..., scheme.template get<types>().unlock_writes());
#endif
        }).detach();
    }

    template <typename W, typename By, template <typename...> class S, typename O, typename C, typename... types>
    inline static constexpr void parallel_by(W& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        // Create a barrier, exit if we have nothing to do
#if !defined(NDEBUG)
        if (scheme.size_from_partition() == 0)
        {
            typename W::barrier_t barrier = waitable.new_waitable(0);
            return;
        }
        typename W::barrier_t barrier = waitable.new_waitable(scheme.size_from_partition() + 1);
#else
        typename W::barrier_t barrier = waitable.new_waitable(scheme.size_from_partition());
        if (scheme.size() == 0)
        {
            return;
        }
#endif

        boost::fibers::fiber([barrier, &scheme, callback = std::move(callback)]() mutable
        {
            auto& component = scheme.template get<By>();
            for (auto obj : component.range_from_partition())
            {
                boost::fibers::fiber([barrier, &scheme, id = obj->id(), callback = std::move(callback)]() mutable
                {
                    std::apply(callback, scheme.search(id));
                    barrier->wait();
                }).detach();
            }

#if !defined(NDEBUG)
            barrier->wait();
            (..., scheme.template get<types>().unlock_writes());
#endif
        }).detach();
    }

private:
    scheme_view_from_partition();
};

template <typename... components>
struct partial_scheme_view_from_partition
{
    template <typename W, template <typename...> class S, typename C, typename... types>
    inline static constexpr void continuous(W& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        static_assert(
            (has_storage_tag(S<types...>::template orchestrator_t<components>::tag, storage_grow::none, storage_layout::continuous) && ...) ||
            (has_storage_tag(S<types...>::template orchestrator_t<components>::tag, storage_grow::none, storage_layout::partitioned) && ...),
            "Use continuous_by when the scheme contains mixed layouts"
            );

        // Create a barrier, exit if we have nothing to do
        typename W::barrier_t barrier = waitable.new_waitable(int(scheme.size_from_partition() > 0));
        if (scheme.size_from_partition() == 0)
        {
            return;
        }

        boost::fibers::fiber([barrier, &scheme, callback = std::move(callback)]() mutable
        {
            auto zip = ::ranges::views::zip(scheme.template get<components>().range_from_partition()...);
            for (auto combined : zip)
            {
                std::apply(callback, combined);
            }

#if !defined(NDEBUG)
            (..., scheme.template get<components>().unlock_writes());
#endif

            barrier->wait();
        }).detach();
    }

    template <typename W, typename By, template <typename...> class S, typename C, typename... types>
    inline static constexpr void continuous_by(W& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        // Create a barrier, exit if we have nothing to do
        typename W::barrier_t barrier = waitable.new_waitable(int(scheme.size_from_partition() > 0));
        if (scheme.size_from_partition() == 0)
        {
            return;
        }

        boost::fibers::fiber([barrier, &scheme, callback = std::move(callback)]() mutable
        {
            auto& component = scheme.template get<By>();
            for (auto obj : component.range_from_partition())
            {
                std::apply(callback, scheme.search(obj->id()));
            }

#if !defined(NDEBUG)
            (..., scheme.template get<components>().unlock_writes());
#endif

            barrier->wait();
        }).detach();
    }

    template <typename W, template <typename...> class S, typename C, typename... types>
    inline static constexpr void parallel(W& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        static_assert(
            (has_storage_tag(S<types...>::template orchestrator_t<components>::tag, storage_grow::none, storage_layout::continuous) && ...) ||
            (has_storage_tag(S<types...>::template orchestrator_t<components>::tag, storage_grow::none, storage_layout::partitioned) && ...),
            "Use parallel_by when the scheme contains mixed layouts"
            );

        // Create a barrier, exit if we have nothing to do
#if !defined(NDEBUG)
        if (scheme.size_from_partition() == 0)
        {
            typename W::barrier_t barrier = waitable.new_waitable(0);
            return;
        }
        typename W::barrier_t barrier = waitable.new_waitable(scheme.size_from_partition() + 1);
#else
        typename W::barrier_t barrier = waitable.new_waitable(scheme.size_from_partition());
        if (scheme.size() == 0)
        {
            return;
        }
#endif

        // TODO(gpascualg): Do we need this outter fiber?
        boost::fibers::fiber([barrier, &scheme, callback = std::move(callback)]() mutable
        {
            auto zip = ::ranges::views::zip(scheme.template get<components>().range_from_partition()...);
            for (auto combined : zip)
            {
                // TODO(gpascualg): Is it safe to get a reference to combined here?
                boost::fibers::fiber([barrier, combined, callback = std::move(callback)]() mutable
                {
                    std::apply(callback, combined);
                    barrier->wait();
                }).detach();
            }

#if !defined(NDEBUG)
            barrier->wait();
            (..., scheme.template get<components>().unlock_writes());
#endif
        }).detach();
    }

    template <typename W, typename By, template <typename...> class S, typename O, typename C, typename... types>
    inline static constexpr void parallel_by(W& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        // Create a barrier, exit if we have nothing to do
#if !defined(NDEBUG)
        if (scheme.size_from_partition() == 0)
        {
            typename W::barrier_t barrier = waitable.new_waitable(0);
            return;
        }
        typename W::barrier_t barrier = waitable.new_waitable(scheme.size_from_partition() + 1);
#else
        typename W::barrier_t barrier = waitable.new_waitable(scheme.size_from_partition());
        if (scheme.size() == 0)
        {
            return;
        }
#endif

        boost::fibers::fiber([barrier, &scheme, callback = std::move(callback)]() mutable
        {
            auto& component = scheme.template get<By>();
            for (auto obj : component.range_from_partition())
            {
                boost::fibers::fiber([barrier, &scheme, id = obj->id(), callback = std::move(callback)]() mutable
                {
                    std::apply(callback, scheme.search(id));
                    barrier->wait();
                }).detach();
            }

#if !defined(NDEBUG)
            barrier->wait();
            (..., scheme.template get<components>().unlock_writes());
#endif
        }).detach();
    }

private:
    partial_scheme_view_from_partition();
};