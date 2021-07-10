#pragma once

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

template <typename T, typename... types>
struct entity_view
{
    template <typename S, typename C>
    inline constexpr void operator()(S& scheme, C&& callback)
    {
        for (auto component : scheme.template get<T>().range())
        {
            tao::apply(callback, tao::tuple(component, scheme.template get<types>().get_derived_or_null(component->id())...));
        }
    }
};
