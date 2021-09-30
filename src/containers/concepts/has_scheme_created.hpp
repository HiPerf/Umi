#pragma once

#include <type_traits>

template<typename, typename T>
struct has_scheme_created {
    static_assert(
        std::integral_constant<T, false>::value,
        "Second template parameter needs to be of function type.");
};

// specialization that does the checking

template<typename C, typename Ret>
struct has_scheme_created<C, Ret()> {
private:

    template<typename T>
    static constexpr auto check(T*) -> typename std::is_same<
        decltype(
            (std::declval<T>().*(static_cast<Ret(T::*)()>(&T::scheme_created)))()
            ),
        Ret    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    >::type;  // attempt to call it and see if the return type is correct

    template<typename>
    static constexpr std::false_type check(...);

    typedef decltype(check<C>(0)) type;

public:
    static constexpr bool value = type::value;
};

template <typename T, typename D>
inline constexpr bool has_scheme_created_v = std::is_base_of_v<T, D> && has_scheme_created<D, void()>::value;
