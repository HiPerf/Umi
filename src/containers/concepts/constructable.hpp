#pragma once

#include <containers/concepts/method_traits.hpp>


template <typename T, typename D, typename... Args>
concept constructable = std::is_base_of_v<T, D> && 
    std::is_same_v<typename detail::tester<decltype(&D::construct)>::result, std::true_type> &&
    requires(D d, Args&&... args)
    {
        { d.construct(std::forward<Args>(args)...) };
    };
