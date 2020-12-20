#pragma once

#include <containers/concepts/method_traits.hpp>


template <typename T, typename D, typename... Args>
concept has_update = std::is_base_of_v<T, D> && 
    std::is_same_v<
        typename detail::tester<
                decltype( static_cast<void(D::*)(typename detail::inner_type<std::decay_t<Args>>::type...)>(&D::update) )
            >::result, 
        std::true_type> &&
    requires(D d, Args&&... args)
    {
        { d.update(std::forward<Args>(args)...) };
    };

template <typename T, typename D, typename... Args>
struct has_update_scope
{
    inline static constexpr bool value = has_update<T, D, Args...>;
};

template <typename T, typename D, typename... Args>
inline constexpr bool has_update_v = has_update_scope<T, D, Args...>::value;

