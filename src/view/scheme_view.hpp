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

public:
    inline void wait()
    {
        boost::fibers::fiber([this, &scheme, callback = std::move(callback)]() mutable
        {
            _updates_mutex.lock();
            _updates_cv.wait(_updates_mutex, [this]() { return _pending_updates == 0; });
            _updates_mutex.unlock();
        }).join();
    }

private:
    std::atomic<uint64_t> _pending_updates;
    boost::fibers::mutex _updates_mutex;
    boost::fibers::condition_variable_any _updates_cv;
};

struct scheme_view
{
    template <template <typename...> class S, typename C, typename... types>
    inline static constexpr void continuous(waitable& waitable, S<types...>& scheme, C&& callback)
    {
        static_assert(
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::continuous) && ...) || 
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::partitioned) && ...),
            "Use continuous_by when the scheme contains mixed layouts"
        );
        
        waitable._pending_updates += scheme.size();

        boost::fibers::fiber([this, &waitable, &scheme, callback = std::move(callback)]() mutable
            {
                for (auto combined : ::ranges::views::zip(scheme.template get<types>().range()...))
                {
                    std::apply(callback, combined);
                    --waitable._pending_updates;
                }
            }).join();
    }

    template <typename By, template <typename...> class S, typename C, typename... types>
    inline static constexpr void continuous_by(waitable& waitable, S<types...>& scheme, C&& callback)
    {   
        waitable._pending_updates += scheme.size();

        boost::fibers::fiber([this, &waitable, &scheme, &component, callback = std::move(callback)]() mutable
            {
                auto& component = scheme.get<By>();
                for (auto obj : component.range())
                {
                    std::apply(callback, scheme.search(obj->id()));
                    --waitable._pending_updates;
                }
            }).join();
    }

    template <template <typename...> class S, typename C, typename... types>
    inline static constexpr void parallel(waitable& waitable, S<types...>& scheme, C&& callback)
    {
        static_assert(
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::continuous) && ...) || 
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::partitioned) && ...),
            "Use parallel_by when the scheme contains mixed layouts"
        );

        waitable._pending_updates += scheme.size();

        // TODO(gpascualg): Do we need this outter fiber?
        boost::fibers::fiber([this, &scheme, callback = std::move(callback)]() mutable
            {
                for (auto combined : ::ranges::views::zip(scheme.template get<types>().range()...))
                {
                    // TODO(gpascualg): Is it safe to get a reference to combined here?
                    boost::fibers::fiber([this, &waitable, combined, callback = std::move(callback)]() mutable
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
    inline static constexpr void parallel_by(waitable& waitable, S<types...>& scheme, C&& callback)
    {
        waitable._pending_updates += scheme.size();

        boost::fibers::fiber([this, &scheme, callback = std::move(callback)]() mutable
            {
                auto& component = scheme.get<By>();
                for (auto obj : component.range())
                {
                    boost::fibers::fiber([this, &waitable, &scheme, id = obj->id(), callback = std::move(callback)]() mutable
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
