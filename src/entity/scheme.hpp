#pragma once

#include "updater/updater.hpp"
#include "updater/updater_batched.hpp"
#include "updater/updater_contiguous.hpp"
#include "updater/updater_all_async.hpp"
#include "storage/storage.hpp"
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
    template <typename O, typename component, typename... Args>
    struct scheme_arguments
    {
        using orchestrator_t = O;

        component& comp;
        tao::tuple<Args...> args;
        bool predicate;
    };
}


template <typename... comps>
struct scheme_store
{
    template <typename T> using orchestrator_t = orchestrator_type<T, comps...>;

    constexpr scheme_store()
    {}

    template <typename T>
    constexpr inline auto get() noexcept -> std::add_lvalue_reference_t<orchestrator_t<T>>
    {
        return tao::get<orchestrator_t<T>>(components);
    }

    tao::tuple<orchestrator_t<comps>...> components;
};

template <typename... comps>
struct tickets_tuple : public tao::tuple<comps...>
{
    using tao::tuple<comps...>::tuple;

    template <typename T>
    inline auto valid() const noexcept
    {
        return tao::get<typename ticket<entity<T>>::ptr>(*this)->valid();
    }

    template <typename T>
    inline auto get() const noexcept
    {
        return tao::get<typename ticket<entity<T>>::ptr>(*this)->get()->derived();
    }
};

template <typename... comps>
struct entity_tuple : public tao::tuple<comps...>
{
    using tao::tuple<comps...>::tuple;

    inline auto tickets() const noexcept
    {
        return tao::apply([](auto... args) {
            return tickets_tuple(args->ticket()...);
        }, downcast());
    }

    inline auto& downcast() const noexcept
    {
        return static_cast<const tao::tuple<comps...>&>(*this);
    }

    template <typename T>
    inline auto get() const noexcept
    {
        return tao::get<typename ticket<entity<T>>::ptr>(downcast())->get()->derived();
    }
};

template <typename... comps>
class scheme
{
    template <typename... T> friend class scheme;

public:
    tao::tuple<std::add_lvalue_reference_t<comps>...> components;

    template <typename T>
    using orchestrator_t = orchestrator_type<T, comps...>;
    using tuple_t = tao::tuple<std::add_pointer_t<typename comps::derived_t>...>;
    using entity_tuple_t = entity_tuple<std::add_pointer_t<typename comps::derived_t>...>;

    template <typename... T>
    constexpr scheme(scheme_store<T...>& store) noexcept :
        components(store.template get<comps>()...)
    {}

    template <template <typename...> typename D, typename... Args>
    constexpr auto make_updater(Args&&... args) noexcept
    {
        return D<std::add_pointer_t<comps>...>(std::forward<Args>(args)..., components_ptr(tao::seq::make_index_sequence<sizeof...(comps)> {}));
    }

    constexpr inline void clear() noexcept
    {
        (get<comps>().clear(), ...);
    }

    template <typename T>
    constexpr inline auto get() const noexcept -> std::add_lvalue_reference_t<orchestrator_t<T>>
    {
        return tao::get<std::add_lvalue_reference_t<orchestrator_t<T>>>(components);
    }

    template <typename T>
    constexpr inline T* get(entity_id_t id) const noexcept
    {
        return get<T>().get_derived_or_null(id);
    }

    constexpr inline auto search(entity_id_t id) const noexcept -> entity_tuple_t
    {
        return entity_tuple_t(get<comps>().get_derived_or_null(id)...);
    }

    template <typename T>
    constexpr inline bool has() const noexcept
    {
        return has_type<T, tao::tuple<comps...>>::value;
    }

    template <typename T>
    constexpr inline void require() const noexcept
    {
        static_assert(has_type<T, tao::tuple<comps...>>::value, "Requirement not met");
    }

    template <typename T, typename... Args, typename = std::enable_if_t<!is_partitioned_storage(orchestrator_t<T>::tag)>>
    constexpr auto args(Args&&... args) noexcept -> detail::scheme_arguments<orchestrator_t<T>, std::add_lvalue_reference_t<orchestrator_t<T>>, std::decay_t<Args>...>
    {
        using D = orchestrator_t<T>;
        require<D>();

        return detail::scheme_arguments<orchestrator_t<T>, std::add_lvalue_reference_t<D>, std::decay_t<Args>...> {
            .comp = tao::get<std::add_lvalue_reference_t<D>>(components),
            .args = tao::tuple<std::decay_t<Args>...>(std::forward<Args>(args)...)
        };
    }

    template <typename T, typename... Args, typename = std::enable_if_t<is_partitioned_storage(orchestrator_t<T>::tag)>>
    constexpr auto args(bool predicate, Args&&... args) noexcept -> detail::scheme_arguments<orchestrator_t<T>, std::add_lvalue_reference_t<orchestrator_t<T>>, std::decay_t<Args>...>
    {
        using D = orchestrator_t<T>;
        require<D>();

        return detail::scheme_arguments<orchestrator_t<T>, std::add_lvalue_reference_t<D>, std::decay_t<Args>...> {
            .comp = tao::get<std::add_lvalue_reference_t<D>>(components),
            .args = tao::tuple<std::decay_t<Args>...>(std::forward<Args>(args)...),
            .predicate = predicate
        };
    }

    template <typename T, typename... Args>
    constexpr T* alloc(uint64_t id, detail::scheme_arguments<orchestrator_t<T>, std::add_lvalue_reference_t<orchestrator_t<T>>, Args...>&& args)
    {
        return create_impl(id, std::move(args));
    }

    template <typename... A>
    auto create(uint64_t id, A&&... scheme_args) noexcept
        requires (... && !std::is_lvalue_reference<A>::value)
    {
        static_assert(sizeof...(comps) == sizeof...(scheme_args), "Incomplete scheme allocation");

        auto entities = entity_tuple_t(create_impl(id, std::move(scheme_args)) ...);

        // Create dynamic content
        auto map = std::make_shared<components_map>(entities);

        // Notify of complete scheme creation
        tao::apply([&map](auto&&... entities) mutable {
            (..., entities->base()->scheme_created(map));
        }, static_cast<tuple_t&>(entities));

        return entities;
    }

    template <typename T>
    constexpr void destroy(T* object)
    {
        destroy(object->template get<typename comps::derived_t>()...);
    }

    template <typename... Args>
    constexpr void destroy(Args... args)
    {
        static_assert(sizeof...(Args) == sizeof...(comps), "Must provide the whole entity components");
        (..., get<orchestrator_t<std::remove_pointer_t<Args>>>().pop(args));
    }
    
    constexpr void destroy(entity_tuple_t entity)
    {
        tao::apply([this](auto... args) {
            destroy(args...);
        }, entity.downcast());
    }

    template <typename T>
    constexpr auto move(scheme<comps...>& to, T* object) noexcept
    {
        return move_impl(to, object->template get<typename comps::derived_t>()...);
    }

    template <typename... Args>
    constexpr auto move(scheme<comps...>& to, Args... args)
    {
        static_assert(sizeof...(Args) == sizeof...(comps), "Must provide the whole entity components");
        return move_impl(to, args...);
    }

    constexpr auto move(scheme<comps...>& to, entity_tuple_t entity)
    {
        return tao::apply([this, &to](auto... args) {
            return move_impl(to, args...);
        }, entity.downcast());
    }

    constexpr inline std::size_t size() const
    {
        return tao::get<0>(components).size();
    }

    //template <typename Sorter, typename UnaryPredicate>
    //void partition(UnaryPredicate&& p)
    //{
    //    ranges::partition(ranges::views::zip(get<comps>().range_as_ref()...),
    //        [p = std::forward<UnaryPredicate>(p)](const auto&... elems) {
    //            return p(std::get<Sorter>(std::forward_as_tuple(elems...)));
    //        });
    //}

    template <typename T>
    auto change_partition(bool p, T* object)
    {
        return change_partition_impl(p, object->template get<typename comps::derived_t>()...);
    }

    template <typename... T, typename... D>
    constexpr auto overlap(scheme_store<T...>& store, scheme<D...>& other) noexcept
    {
        using W = without_duplicates<scheme, scheme<D..., comps...>>;
        return W{ store };
    }

private:
    template <std::size_t... I>
    inline constexpr auto components_ptr(std::index_sequence<I...>) noexcept
    {
        return tao::tuple(&tao::get<I>(components)...);
    }

    template <typename... Ts>
    inline constexpr auto move_impl(scheme<comps...>& to, Ts&&... entities) noexcept
    {
        return tao::apply([this](auto... entities) mutable {
            (entities->base()->scheme_information(*this), ...);
            return entity_tuple_t(entities...);
        }, tao::tuple(get<comps>().move(entities->ticket(), to.get<comps>())...));
    }

    template <typename... Ts>
    inline auto change_partition_impl(bool p, Ts... objects)
    {
        return tao::tuple(get<comps>().change_partition(p, objects)...);
    }

    template <typename T>
    constexpr auto create_impl(uint64_t id, T&& scheme_args) noexcept
    {
        // Create by invoking with arguments
        auto entity = tao::apply([&scheme_args, &id](auto&&... args) {
            if constexpr (is_partitioned_storage(T::orchestrator_t::tag))
            {
                return scheme_args.comp.push(scheme_args.predicate, id, std::forward<std::decay_t<decltype(args)>>(args)...);
            }
            else
            {
                return scheme_args.comp.push(id, std::forward<std::decay_t<decltype(args)>>(args)...);
            }
        }, scheme_args.args);

        // Notify of creation
        entity->base()->scheme_information(*this);

        return entity;
    }
};


template <typename... comps>
struct scheme_maker
{
    template <typename... T>
    inline auto constexpr operator()(scheme_store<T...>& store) noexcept
    {
        if constexpr (sizeof...(comps) == 0)
        {
            using W = without_duplicates<scheme, scheme<typename scheme_store<T...>::template orchestrator_t<T>...>>;
            return W{ store };
        }
        else
        {
            using W = without_duplicates<scheme, scheme<typename scheme_store<T...>::template orchestrator_t<comps>...>>;
            return W{ store };
        }
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
