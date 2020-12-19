#pragma once


namespace detail
{
    template<typename>
    struct tester
    {
        using result = std::false_type;
    };
    
    template<class T, class U>
    struct tester<U T::*>
    {
        using result = std::true_type;
    };
}
