#pragma once

#include <containers/concepts/method_traits.hpp>


template <typename T, typename D, typename... Args>
concept has_sync = std::is_base_of_v<T, D> && 
    std::is_same_v<
        typename detail::tester<
                decltype( static_cast<void(D::*)(typename detail::inner_type<std::decay_t<Args>>::type...)>(&D::sync) )
            >::result, 
        std::true_type> &&
    requires(D d, Args&&... args)
    {
        { d.sync(std::forward<Args>(args)...) };
    };

template <typename T, typename D, typename... Args>
struct has_sync_scope
{
    inline static constexpr bool value = has_sync<T, D, Args...>;
};

template <typename T, typename D, typename... Args>
inline constexpr bool has_sync_v = has_sync_scope<T, D, Args...>::value;
