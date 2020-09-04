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
