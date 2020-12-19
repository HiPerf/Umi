#pragma once

#include <containers/concepts/method_traits.hpp>


template <typename T, typename D, typename... Args>
concept has_update = std::is_base_of_v<T, D> && 
    //std::is_same_v<typename detail::tester<decltype(&D::update)>::result, std::true_type> &&
    // std::is_same_v<typename detail::tester<decltype( std::declval<D>.update()(std::declval<Args>()...) )(D::*)(Args...)>::result, std::true_type> &&
    std::is_same_v<typename detail::tester<std::invoke_result_t<decltype(static_cast<void(D::*)(Args...)>(&D::update)), D, Args...> (D::*)(Args...)>::result, std::true_type> && 
    requires(D d, Args&&... args)
    {
        { d.update(std::forward<Args>(args)...) };
    };

