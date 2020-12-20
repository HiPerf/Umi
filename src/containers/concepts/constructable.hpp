#pragma once

#include <containers/concepts/method_traits.hpp>


template <typename T, typename D, typename... Args>
concept constructable = std::is_base_of_v<T, D> && 
    std::is_same_v<
        typename detail::tester<
                decltype( static_cast<void(D::*)(typename detail::inner_type<std::decay_t<Args>>::type...)>(&D::construct) )
            >::result, 
        std::true_type> &&
    requires(D d, Args&&... args)
    {
        { d.construct(std::forward<Args>(args)...) };
    };

template <typename T, typename D, typename... Args>
struct constructable_scope
{
    inline static constexpr bool value = constructable<T, D, Args...>;
};

template <typename T, typename D, typename... Args>
inline constexpr bool constructable_v = constructable_scope<T, D, Args...>::value;
