#pragma once

#include <tao/tuple/tuple.hpp>

#include <type_traits>


template <class Candidate, class In>
struct base_dic;

template <class Candidate, class InCar, class... InCdr>
struct base_dic<Candidate, tao::tuple<InCar, InCdr...>>
{
  using type = typename std::conditional<
    std::is_same<Candidate, typename InCar::derived_t>::value
    , typename base_dic<InCar, tao::tuple<>>::type
    , typename base_dic<Candidate, tao::tuple<InCdr...>>::type
  >::type;
};

template <class Candidate>
struct base_dic<Candidate, tao::tuple<>>
{
  using type = Candidate;
};

//
//template <bool B, template <typename...> class TrueTemplate, template <typename...> class FalseTemplate, typename ArgsTuple>
//struct lazy_conditional;
//
//template <template <typename...> class TrueTemplate, template <typename...> class FalseTemplate, typename ... Args>
//struct lazy_conditional<true, TrueTemplate, FalseTemplate, std::tuple<Args...>>
//{
//    using type = TrueTemplate<Args...>;
//};
//
//template <template <typename...> class TrueTemplate, template <typename...> class FalseTemplate, typename ... Args>
//struct lazy_conditional<false, TrueTemplate, FalseTemplate, std::tuple<Args...>>
//{
//    using type = FalseTemplate<Args...>;
//};
//
//template <bool Valid, class Candidate, class In>
//struct extended_type_impl;
//
//template <bool Valid, class Candidate, class InCar, class... InCdr>
//struct extended_type_impl<Valid, Candidate, tao::tuple<InCar, InCdr...>>
//{
//    using type = typename lazy_conditional<
//        std::is_same_v<Candidate, typename InCar::base_t> || std::is_same_v<Candidate, typename InCar::derived_t>,
//        typename extended_type_impl<true, InCar, tao::tuple<>>::type,
//        typename extended_type_impl<false, Candidate, tao::tuple<InCdr...>>::type
//    >::type;
//};
//
//template <class Candidate>
//struct extended_type_impl<true, Candidate, tao::tuple<>>
//{
//    using type = typename Candidate;
//};
//
//template <class Candidate>
//struct extended_type_impl<false, Candidate, tao::tuple<>>
//{
//    using type = typename Candidate;
//
//    
//};
//
//template <class Candidate, class In>
//using extended_type = extended_type_impl<false, Candidate, In>;


template <typename Candidate, typename InCar, typename... InCdr>
inline constexpr const auto& orchestrator_type_impl()
{
    if constexpr (
        std::is_same_v<Candidate, InCar::orchestrator_t> ||         // We are given an orchestrator and we might have storages 
        std::is_same_v<Candidate, InCar> ||                         // Provided candidate is already an storage type
        std::is_same_v<Candidate, typename InCar::base_t> ||        // Provided type is the base entity<X> type
        std::is_same_v<Candidate, typename InCar::derived_t>)       // Provided type is the derived X type
    {
        return std::declval<InCar::orchestrator_t>();
    }
    else if constexpr (sizeof...(InCdr) == 0)
    {
        //static_assert(false, "Provided type is not a storage type");
        return std::declval<InCar>();
    }
    else
    {
        return orchestrator_type_impl<Candidate, InCdr...>();
    }
}

template <typename Candidate, typename... In>
using orchestrator_type = std::remove_const_t<std::remove_reference_t<decltype(orchestrator_type_impl<Candidate, In...>())>>;
