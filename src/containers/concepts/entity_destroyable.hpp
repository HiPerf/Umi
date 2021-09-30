#pragma once

#include <type_traits>

template<typename, typename T>
struct entity_destroyable {
    static_assert(
        std::integral_constant<T, false>::value,
        "Second template parameter needs to be of function type.");
};

// specialization that does the checking

template<typename C, typename Ret, typename... Args>
struct entity_destroyable<C, Ret(Args...)> {
private:

    template<typename T>
    static constexpr auto check(T*) -> typename std::is_same<
        decltype(
            (std::declval<T>().*(static_cast<Ret(T::*)(typename std::unwrap_ref_decay_t<Args>...)>(&T::entity_destroy)))(std::declval<std::unwrap_ref_decay_t<Args>>()...)
            ),
        Ret    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    >::type;  // attempt to call it and see if the return type is correct

    template<typename>
    static constexpr std::false_type check(...);

    typedef decltype(check<C>(0)) type;

public:
    static constexpr bool value = type::value;
};

template <typename T, typename D, typename... Args>
inline constexpr bool entity_destroyable_v = std::is_base_of_v<T, D> && entity_destroyable<D, void(Args...)>::value;
