#pragma once

#include <containers/concepts/method_traits.hpp>


template <typename T, typename D, template <typename...> typename S, typename... components>
concept has_scheme_information = std::is_base_of_v<T, D> && 
    std::is_same_v<typename detail::tester<decltype(&D::scheme_information)>::result, std::true_type> &&
    requires(D d, S<components...>& s)
    {
        { d.scheme_information(s) };
    };
