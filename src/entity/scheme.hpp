#pragma once

#include "updater/updater.hpp"
#include "updater/updater_batched.hpp"
#include "updater/updater_contiguous.hpp"
#include "updater/updater_all_async.hpp"
#include "traits/base_dic.hpp"
#include "traits/has_type.hpp"
#include "traits/tuple.hpp"
#include "traits/without_duplicates.hpp"

#include <range/v3/algorithm/partition.hpp>
#include <range/v3/view/zip.hpp>

#include <tao/tuple/tuple.hpp>


template <typename... vectors>
struct scheme;

template <typename... vectors>
struct scheme_store;


namespace detail
{
    template <typename component, typename... Args>
    struct scheme_arguments
    {
        component& comp;
        tao::tuple<Args...> args;
    };
}


template <typename... vectors>
struct scheme_store
{
    template <typename T> using dic_t = typename base_dic<T, tao::tuple<vectors...>>::type;

    constexpr scheme_store()
    {}

    template <typename T>
    constexpr inline auto get() noexcept -> std::add_lvalue_reference_t<typename base_dic<T, tao::tuple<vectors...>>::type>
    {
        using D = typename base_dic<T, tao::tuple<vectors...>>::type;
        return tao::get<D>(components);
    }

    tao::tuple<vectors...> components;
};

template <typename... vectors>
class scheme
{
    template <typename... T> friend class scheme;

public:
    tao::tuple<std::add_lvalue_reference_t<vectors>...> components;

    using tuple_t = tao::tuple<std::add_pointer_t<typename vectors::derived_t>...>;

    template <typename... T>
    constexpr scheme(scheme_store<T...>& store) noexcept :
        components(store.template get<vectors>()...)
    {}

    template <template <typename...> typename D, typename... Args>
    constexpr auto make_updater(Args&&... args) noexcept
    {
        return D<std::add_pointer_t<vectors>...>(std::forward<Args>(args)..., components_ptr(tao::seq::make_index_sequence<sizeof...(vectors)> {}));
    }

    constexpr inline void clear() noexcept
    {
        (get<vectors>().clear(), ...);
    }

    template <typename T>
    constexpr inline auto get() const noexcept -> std::add_lvalue_reference_t<typename base_dic<T, tao::tuple<vectors...>>::type>
    {
        using D = typename base_dic<T, tao::tuple<vectors...>>::type;
        return tao::get<std::add_lvalue_reference_t<D>>(components);
    }

    template <typename T>
    constexpr inline T* get(entity_id_t id) const noexcept
    {
        return get<T>().get_derived_or_null(id);
    }

    constexpr inline auto search(uint64_t id) const noexcept -> tao::tuple<std::add_pointer_t<typename vectors::derived_t>...>
    {
        return tao::tuple(get<vectors>().get_derived_or_null(id)...);
    }

    template <typename T>
    constexpr inline bool has() const noexcept
    {
        return has_type<T, tao::tuple<vectors...>>::value;
    }

    template <typename T>
    constexpr inline void require() const noexcept
    {
        static_assert(has_type<T, tao::tuple<vectors...>>::value, "Requirement not met");
    }

    template <typename T, typename... Args>
    constexpr auto args(Args&&... args) noexcept -> detail::scheme_arguments<std::add_lvalue_reference_t<typename base_dic<T, tao::tuple<vectors...>>::type>, std::decay_t<Args>...>
    {
        using D = typename base_dic<T, tao::tuple<vectors...>>::type;
        require<D>();

        return detail::scheme_arguments<std::add_lvalue_reference_t<typename base_dic<T, tao::tuple<vectors...>>::type>, std::decay_t<Args>...> {
            .comp = tao::get<std::add_lvalue_reference_t<D>>(components),
            .args = tao::tuple<std::decay_t<Args>...>(std::forward<Args>(args)...)
        };
    }

    template <typename T, typename... Args>
    constexpr T* alloc(uint64_t id, Args&&... constructor_args)
    {
        auto scheme_args = args<T>(std::forward<Args>(constructor_args)...);
        return tao::apply([this, &scheme_args, &id](auto&&... args) mutable {
            auto entity = scheme_args.comp.alloc(id, std::forward<std::decay_t<decltype(args)>>(args)...);
            entity->base()->scheme_information(*this);
            return entity;
        }, scheme_args.args);
    }

    template <typename T>
    constexpr void free(T* object)
    {
        free_impl(object->template get<typename vectors::derived_t>()...);
    }

    template <typename T>
    constexpr void free_with_partition(bool p, T* object)
    {
        free_with_partition_impl(p, object->template get<typename vectors::derived_t>()...);
    }

    template <typename T>
    constexpr auto move(scheme<vectors...>& to, T* object) noexcept
    {
        return move_impl(to, object->template get<typename vectors::derived_t>()...);
    }

    template <typename T>
    constexpr auto move_with_partition(bool p, scheme<vectors...>& to, T* object) noexcept
    {
        return move_with_partition_impl(p, to, object->template get<typename vectors::derived_t>()...);
    }

    constexpr inline std::size_t size() const
    {
        return tao::get<0>(components).size();
    }

    template <typename Sorter, typename UnaryPredicate>
    void partition(UnaryPredicate&& p)
    {
        ranges::partition(ranges::views::zip(get<vectors>().range_as_ref()...),
            [p = std::forward<UnaryPredicate>(p)](const auto&... elems) {
                return p(std::get<Sorter>(std::forward_as_tuple(elems...)));
            });
    }

    template <typename T>
    auto partition_change(bool p, T* object)
    {
        return partition_change_impl(p, object->template get<typename vectors::derived_t>()...);
    }

    template <typename... T, typename... D>
    constexpr auto overlap(scheme_store<T...>& store, scheme<D...>& other) noexcept
    {
        using W = without_duplicates<scheme, scheme<D..., vectors...>>;
        return W{ store };
    }

private:
    template <std::size_t... I>
    inline constexpr auto components_ptr(std::index_sequence<I...>) noexcept
    {
        return tao::tuple(&tao::get<I>(components)...);
    }

    template <typename... Ts>
    inline constexpr void free_impl(Ts... objects)
    {
        (get<vectors>().free(objects), ...);
    }

    template <typename... Ts>
    inline constexpr void free_with_partition_impl(bool p, Ts... objects)
    {
        (get<vectors>().free_with_partition(p, objects), ...);
    }

    template <typename... Ts>
    inline constexpr auto move_impl(scheme<vectors...>& to, Ts&&... entities) noexcept
    {
        return tao::apply([this](auto... entities) mutable {
            (entities->base()->scheme_information(*this), ...);
            return tao::tuple(entities...);
        }, tao::tuple(get<vectors>().move(entities->ticket(), to.get<vectors>())...));
    }

    template <typename... Ts>
    inline constexpr auto move_with_partition_impl(bool p, scheme<vectors...>& to, Ts... entities) noexcept
    {
        return tao::apply([this](auto... entities) mutable {
            (entities->base()->scheme_information(*this), ...);
            return tao::tuple(entities...);
        }, tao::tuple(get<vectors>().move_with_partition(p, entities->ticket(), to.get<vectors>())...));
    }

    template <typename... Ts>
    inline auto partition_change_impl(bool p, Ts... objects)
    {
        return tao::tuple(get<vectors>().partition_change(p, objects)...);
    }

};


template <typename... vectors>
struct scheme_maker
{
    template <typename... T>
    inline auto constexpr operator()(scheme_store<T...>& store) noexcept
    {
        using W = without_duplicates<scheme, scheme<typename scheme_store<T...>::template dic_t<vectors>...>>;
        return W{ store };
    }
};

template <typename... T, typename A, typename B, typename... O>
constexpr inline auto overlap(scheme_store<T...>& store, A&& a, B&& b, O&&... other) noexcept
{
    if constexpr (sizeof...(other) == 0)
    {
        return a.overlap(store, b);
    }
    else
    {
        return overlap(store, a.overlap(b), std::forward<O>(other)...);
    }
}
