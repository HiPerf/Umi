#pragma once

#include "storage/storage.hpp"

#include <boost/fiber/fiber.hpp>
#include <boost/fiber/mutex.hpp>
#include <boost/fiber/condition_variable.hpp>
#include <boost/range.hpp>
#include <boost/range/combine.hpp>

#include <range/v3/view/zip.hpp>
#include <tao/tuple/tuple.hpp>

#include <tuple>


struct waitable
{
    friend struct scheme_view;
    friend struct scheme_view_until_partition;
    friend struct scheme_view_from_partition;

public:
    waitable() = default;

    waitable(waitable&& other) noexcept :
        _pending_updates(static_cast<uint64_t>(other._pending_updates)),
        _updates_mutex(),
        _updates_cv()
    {
        assert(other.done() && "Cannot move while updates are pending");
    }

    waitable& operator=(waitable&& rhs) noexcept
    {
        assert(rhs.done() && "Cannot move while updates are pending");
        _pending_updates = static_cast<uint64_t>(rhs._pending_updates);
        return *this;
    }

    inline void wait() noexcept
    {
        boost::fibers::fiber([this]() mutable
        {
            _updates_mutex.lock();
            _updates_cv.wait(_updates_mutex, [this]() { return _pending_updates == 0; });
            _updates_mutex.unlock();
        }).join();
    }

    inline bool done() const noexcept
    {
        return _pending_updates == 0;
    }

private:
    std::atomic<uint64_t> _pending_updates;
    boost::fibers::mutex _updates_mutex;
    boost::fibers::condition_variable_any _updates_cv;
};

struct scheme_view
{
    template <template <typename...> class S, typename C, typename... types>
    inline static constexpr void continuous(waitable& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        static_assert(
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::continuous) && ...) || 
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::partitioned) && ...),
            "Use continuous_by when the scheme contains mixed layouts"
        );
        
        waitable._pending_updates += scheme.size();

        boost::fibers::fiber([&waitable, &scheme, callback = std::move(callback)]() mutable
            {
                for (auto combined : ::ranges::views::zip(scheme.template get<types>().range()...))
                {
                    std::apply(callback, combined);
                    --waitable._pending_updates;
                }
            }).join();
    }

    template <typename By, template <typename...> class S, typename C, typename... types>
    inline static constexpr void continuous_by(waitable& waitable, S<types...>& scheme, C&& callback) noexcept
    {   
        waitable._pending_updates += scheme.size();

        boost::fibers::fiber([&waitable, &scheme, callback = std::move(callback)]() mutable
            {
                auto& component = scheme.template get<By>();
                for (auto obj : component.range())
                {
                    std::apply(callback, scheme.search(obj->id()));
                    --waitable._pending_updates;
                }
            }).join();
    }

    template <template <typename...> class S, typename C, typename... types>
    inline static constexpr void parallel(waitable& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        static_assert(
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::continuous) && ...) || 
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::partitioned) && ...),
            "Use parallel_by when the scheme contains mixed layouts"
        );

        waitable._pending_updates += scheme.size();

        // TODO(gpascualg): Do we need this outter fiber?
        boost::fibers::fiber([&waitable, &scheme, callback = std::move(callback)]() mutable
            {
                for (auto combined : ::ranges::views::zip(scheme.template get<types>().range()...))
                {
                    // TODO(gpascualg): Is it safe to get a reference to combined here?
                    boost::fibers::fiber([&waitable, combined, callback = std::move(callback)]() mutable
                        {
                            std::apply(callback, combined);
                            
                            if (--waitable._pending_updates == 0)
                            {
                                waitable._updates_cv.notify_all();
                            }
                        }).detach();
                }
            }).join();
    }

    template <typename By, template <typename...> class S, typename O, typename C, typename... types>
    inline static constexpr void parallel_by(waitable& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        waitable._pending_updates += scheme.size();

        boost::fibers::fiber([&waitable, &scheme, callback = std::move(callback)]() mutable
            {
                auto& component = scheme.template get<By>();
                for (auto obj : component.range())
                {
                    boost::fibers::fiber([&waitable, &scheme, id = obj->id(), callback = std::move(callback)]() mutable
                        {
                            std::apply(callback, scheme.search(id));
                            
                            if (--waitable._pending_updates == 0)
                            {
                                waitable._updates_cv.notify_all();
                            }
                        }).detach();
                }
            }).join();
    }

private:
    scheme_view();
};

struct scheme_view_until_partition
{
    template <template <typename...> class S, typename C, typename... types>
    inline static constexpr void continuous(waitable& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        static_assert(
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::continuous) && ...) ||
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::partitioned) && ...),
            "Use continuous_by when the scheme contains mixed layouts"
            );

        waitable._pending_updates += scheme.size();

        boost::fibers::fiber([&waitable, &scheme, callback = std::move(callback)]() mutable
        {
            for (auto combined : ::ranges::views::zip(scheme.template get<types>().range_until_partition()...))
            {
                std::apply(callback, combined);
                --waitable._pending_updates;
            }
        }).join();
    }

    template <typename By, template <typename...> class S, typename C, typename... types>
    inline static constexpr void continuous_by(waitable& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        waitable._pending_updates += scheme.size();

        boost::fibers::fiber([&waitable, &scheme, callback = std::move(callback)]() mutable
        {
            auto& component = scheme.template get<By>();
            for (auto obj : component.range_until_partition())
            {
                std::apply(callback, scheme.search(obj->id()));
                --waitable._pending_updates;
            }
        }).join();
    }

    template <template <typename...> class S, typename C, typename... types>
    inline static constexpr void parallel(waitable& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        static_assert(
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::continuous) && ...) ||
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::partitioned) && ...),
            "Use parallel_by when the scheme contains mixed layouts"
            );

        waitable._pending_updates += scheme.size();

        // TODO(gpascualg): Do we need this outter fiber?
        boost::fibers::fiber([&waitable, &scheme, callback = std::move(callback)]() mutable
        {
            for (auto combined : ::ranges::views::zip(scheme.template get<types>().range_until_partition()...))
            {
                // TODO(gpascualg): Is it safe to get a reference to combined here?
                boost::fibers::fiber([&waitable, combined, callback = std::move(callback)]() mutable
                {
                    std::apply(callback, combined);

                    if (--waitable._pending_updates == 0)
                    {
                        waitable._updates_cv.notify_all();
                    }
                }).detach();
            }
        }).join();
    }

    template <typename By, template <typename...> class S, typename O, typename C, typename... types>
    inline static constexpr void parallel_by(waitable& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        waitable._pending_updates += scheme.size();

        boost::fibers::fiber([&waitable, &scheme, callback = std::move(callback)]() mutable
        {
            auto& component = scheme.template get<By>();
            for (auto obj : component.range_until_partition())
            {
                boost::fibers::fiber([&waitable, &scheme, id = obj->id(), callback = std::move(callback)]() mutable
                {
                    std::apply(callback, scheme.search(id));

                    if (--waitable._pending_updates == 0)
                    {
                        waitable._updates_cv.notify_all();
                    }
                }).detach();
            }
        }).join();
    }

private:
    scheme_view_until_partition();
};

struct scheme_view_from_partition
{
    template <template <typename...> class S, typename C, typename... types>
    inline static constexpr void continuous(waitable& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        static_assert(
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::continuous) && ...) ||
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::partitioned) && ...),
            "Use continuous_by when the scheme contains mixed layouts"
            );

        waitable._pending_updates += scheme.size();

        boost::fibers::fiber([&waitable, &scheme, callback = std::move(callback)]() mutable
        {
            for (auto combined : ::ranges::views::zip(scheme.template get<types>().range_from_partition()...))
            {
                std::apply(callback, combined);
                --waitable._pending_updates;
            }
        }).join();
    }

    template <typename By, template <typename...> class S, typename C, typename... types>
    inline static constexpr void continuous_by(waitable& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        waitable._pending_updates += scheme.size();

        boost::fibers::fiber([&waitable, &scheme, callback = std::move(callback)]() mutable
        {
            auto& component = scheme.template get<By>();
            for (auto obj : component.range_from_partition())
            {
                std::apply(callback, scheme.search(obj->id()));
                --waitable._pending_updates;
            }
        }).join();
    }

    template <template <typename...> class S, typename C, typename... types>
    inline static constexpr void parallel(waitable& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        static_assert(
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::continuous) && ...) ||
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::partitioned) && ...),
            "Use parallel_by when the scheme contains mixed layouts"
            );

        waitable._pending_updates += scheme.size();

        // TODO(gpascualg): Do we need this outter fiber?
        boost::fibers::fiber([&waitable, &scheme, callback = std::move(callback)]() mutable
        {
            for (auto combined : ::ranges::views::zip(scheme.template get<types>().range_from_partition()...))
            {
                // TODO(gpascualg): Is it safe to get a reference to combined here?
                boost::fibers::fiber([&waitable, combined, callback = std::move(callback)]() mutable
                {
                    std::apply(callback, combined);

                    if (--waitable._pending_updates == 0)
                    {
                        waitable._updates_cv.notify_all();
                    }
                }).detach();
            }
        }).join();
    }

    template <typename By, template <typename...> class S, typename O, typename C, typename... types>
    inline static constexpr void parallel_by(waitable& waitable, S<types...>& scheme, C&& callback) noexcept
    {
        waitable._pending_updates += scheme.size();

        boost::fibers::fiber([&waitable, &scheme, callback = std::move(callback)]() mutable
        {
            auto& component = scheme.template get<By>();
            for (auto obj : component.range_from_partition())
            {
                boost::fibers::fiber([&waitable, &scheme, id = obj->id(), callback = std::move(callback)]() mutable
                {
                    std::apply(callback, scheme.search(id));

                    if (--waitable._pending_updates == 0)
                    {
                        waitable._updates_cv.notify_all();
                    }
                }).detach();
            }
        }).join();
    }

private:
    scheme_view_from_partition();
};
