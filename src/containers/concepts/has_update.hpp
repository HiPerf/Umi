#pragma once

#include <type_traits>

template <typename T, typename D, typename... Args>
concept has_update = std::is_base_of_v<T, D> && 
    requires(D d, Args&&... args)
    {
        { (d.*(static_cast<void(D::*)(typename std::unwrap_ref_decay_t<Args>...)>(&D::update)))(std::forward<Args>(args)...) };
    };

template <typename T, typename D, typename... Args>
struct has_update_scope
{
    inline static constexpr bool value = has_update<T, D, Args...>;
};

template <typename T, typename D, typename... Args>
inline constexpr bool has_update_v = has_update_scope<T, D, Args...>::value;