#pragma once

#include <containers/concepts/method_traits.hpp>


template <typename T, typename D>
concept has_scheme_created = std::is_base_of_v<T, D> && 
    std::is_same_v<typename detail::tester<decltype(&D::scheme_created)>::result, std::true_type> &&
    requires(D d)
    {
        { d.scheme_created() };
    };

