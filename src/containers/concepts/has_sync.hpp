#pragma once

#include <containers/concepts/method_traits.hpp>


template <typename T, typename D, typename... Args>
concept has_sync = std::is_base_of_v<T, D> && 
    std::is_same_v<typename detail::tester<decltype(&D::sync)>::result, std::true_type> &&
    requires(D d, Args&&... args)
    {
        { d.sync(std::forward<Args>(args)...) };
    };

