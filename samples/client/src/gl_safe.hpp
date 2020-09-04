#pragma once

#include <tao/tuple/tuple.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdlib.h>

template <typename F, typename... Args>
auto inline GL_SAFE(F&& f, Args&&... args) -> decltype(f(std::forward<Args>(args)...))
{
    using ret_t = decltype(f(std::forward<Args>(args)...));

    if constexpr (std::is_same_v<void, ret_t>)
    {
        f(std::forward<Args>(args)...);

        auto code = glGetError();
        if (code != GL_NO_ERROR)
        {
            abort();
        }
    }
    else
    {
        ret_t x = f(std::forward<Args>(args)...);

        auto code = glGetError();
        if (code != GL_NO_ERROR)
        {
            abort();
        }

        return x;
    }
}

template <class T, class Tuple, std::size_t... I>
constexpr bool GL_SAFE_EX_impl(T x, Tuple&& t, std::index_sequence<I...>)
{
    return ((tao::get<I>(std::forward<Tuple>(t)) == x) || ...);
}

template <typename F, typename... TP, typename... Args>
auto inline GL_SAFE_EX(tao::tuple<TP...> err, F&& f, Args&&... args) -> decltype(f(std::forward<Args>(args)...))
{
    auto x = GL_SAFE(std::forward<F>(f), std::forward<Args>(args)...);

    if (GL_SAFE_EX_impl(x, err, tao::seq::make_index_sequence<tao::tuple_size<decltype(err)>::value>{}))
    {
        abort();
    }

    return x;
}
