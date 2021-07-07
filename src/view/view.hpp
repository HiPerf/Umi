#pragma once

#include <boost/range.hpp>
#include <boost/range/combine.hpp>

#include <tuple>


template <typename... types>
struct view
{
    template <typename S, typename C>
    inline constexpr void operator()(const S& scheme, C&& callback)
    {
        std::apply(std::move(callback), boost::range::combine(scheme.get<types>()...));
    }
};
