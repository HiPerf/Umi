#pragma once

#include <containers/concepts/method_traits.hpp>


template <typename T, typename D, typename... Args>
concept entity_destroyable = std::is_base_of_v<T, D> && 
    std::is_same_v<
        typename detail::tester<
                decltype( static_cast<void(D::*)(typename detail::inner_type<std::decay_t<Args>>::type...)>(&D::entity_destroy) )
            >::result, 
        std::true_type> &&
    requires(D d, Args&&... args)
    {
        { d.entity_destroy(std::forward<Args>(args)...) };
    };

template <typename T, typename D, typename... Args>
struct entity_destroyable_scope
{
    inline static constexpr bool value = entity_destroyable<T, D, Args...>;
};

template <typename T, typename D, typename... Args>
inline constexpr bool entity_destroyable_v = entity_destroyable_scope<T, D, Args...>::value;
