#pragma once

#include <boost/range.hpp>
#include <boost/range/combine.hpp>

#include <tuple>


template <typename C, typename It, typename... Its>
inline constexpr void zip_i(C&& callback, It it, It end, Its&&... its)
{
    if (it != end)
    {
        callback(*it, *its...);
        zip_i(std::forward<C>(callback), ++it, end, ++its...);
    }
}

template <typename C, typename Range, typename... Ranges>
inline constexpr void zip(C&& callback, Range r, Ranges&&... rs)
{
    zip_i(std::forward<C>(callback), begin(r), end(r), begin(rs)...);
}

template <typename... types>
struct view
{
    template <typename S, typename C>
    inline constexpr void operator()(S& scheme, C&& callback)
    {
        zip(std::forward<C>(callback), scheme.template get<types>().range()...);
        // for (auto combined : boost::range::combine(scheme.template get<types>().range()...))
        // {
        //     std::apply(callback, combined);
        // }
    }
};

