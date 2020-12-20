#pragma once

#include <containers/concepts/method_traits.hpp>


template <typename T, typename D, template <typename...> typename S, typename... components>
concept has_scheme_information = std::is_base_of_v<T, D> && 
    std::is_same_v<
        typename detail::tester<
                decltype( static_cast<void(D::*)(S<components...>&)>(&D::scheme_information) )
            >::result, 
        std::true_type> &&
    requires(D d, S<components...>& s)
    {
        { d.scheme_information(s) };
    };

template <typename T, typename D, template <typename...> typename S, typename... components>
struct has_scheme_information_scope
{
    inline static constexpr bool value = has_scheme_information<T, D, S, components...>;
};

template <typename T, typename D, template <typename...> typename S, typename... components>
inline constexpr bool has_scheme_information_v = has_scheme_information_scope<T, D, S, components...>::value;
