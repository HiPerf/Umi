#pragma once

#include <boost/fiber/fiber.hpp>
#include <boost/fiber/mutex.hpp>
#include <boost/fiber/condition_variable.hpp>
#include <boost/range.hpp>
#include <boost/range/combine.hpp>

#include <range/v3/view/zip.hpp>
#include <tao/tuple/tuple.hpp>

#include <tuple>


template <typename... types>
struct view
{
    template <typename S, typename C>
    inline constexpr void operator()(S& scheme, C&& callback)
    {
        for (auto combined : ::ranges::views::zip(scheme.template get<types>().range()...))
        {
            std::apply(callback, combined);
        }
    }
};


template <typename S, typename C>
struct view_spec
{
    using scheme = S;
    using component = C;

    view_spec(S& s) : _s(s)
    {}

    S& _s;
};

template <typename... types>
struct multi_view
{
    template <typename C>
    inline constexpr void operator()(types... s, C&& callback)
    {
        for (auto combined : ::ranges::views::zip(s._s.template get<typename types::component>().range()...))
        {
            std::apply(callback, combined);
        }
    }
};

struct entity_view
{
    template <template <typename...> class S, typename C, typename... types>
    inline constexpr void operator()(S<types...>& scheme, C&& callback)
    {
        for (auto combined : ::ranges::views::zip(scheme.template get<types>().range()...))
        {
            std::apply(callback, combined);
        }
    }

    template <template <typename...> class S, typename C, typename... types>
    inline constexpr void contiguous(S<types...>& scheme, C&& callback)
    {
        _pending_updates = scheme.size();

        boost::fibers::fiber([this, &scheme, callback = std::move(callback)]() mutable
            {
                for (auto combined : ::ranges::views::zip(scheme.template get<types>().range()...))
                {
                    std::apply(callback, combined);
                    --_pending_updates;
                }
            }).join();
    }

    template <template <typename...> class S, typename C, typename... types>
    inline constexpr void parallel(S<types...>& scheme, C&& callback)
    {
        _pending_updates = scheme.size();

        boost::fibers::fiber([this, &scheme, callback = std::move(callback)]() mutable
            {
                for (auto combined : ::ranges::views::zip(scheme.template get<types>().range()...))
                {
                    // TODO(gpascualg): Is it safe to get a reference to combined here?
                    boost::fibers::fiber([this, combined, callback = std::move(callback)]() mutable
                        {
                            std::apply(callback, combined);

                            _updates_mutex.lock();
                            --_pending_updates;
                            _updates_mutex.unlock();

                            if (_pending_updates == 0)
                            {
                                _updates_cv.notify_all();
                            }
                        }).detach();
                }

                // Wait for updates to end
                _updates_mutex.lock();
                _updates_cv.wait(_updates_mutex, [this]() { return _pending_updates == 0; });
                _updates_mutex.unlock();
            }).join();
    }

private:
    uint64_t _pending_updates;
    boost::fibers::mutex _updates_mutex;
    boost::fibers::condition_variable_any _updates_cv;
};
