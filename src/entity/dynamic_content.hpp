#pragma once

#include "containers/ticket.hpp"
#include "common/tao.hpp"

#include <ctti/type_id.hpp>
#include <inttypes.h>


template <typename D> class entity;

class dynamic_content
{
public:
    template <typename T> using bare_t = typename std::remove_pointer_t<std::decay_t<T>>;
    template <typename T> using ticket_of = typename ::ticket<entity<bare_t<T>>>::ptr;
    template <typename... Ts> using tickets_tuple_t = tao::tuple<ticket_of<Ts>...>;

    constexpr dynamic_content() :
        _type(ctti::type_id<void>()),
        _data(nullptr)
    {}

    template <typename... Ts>
    constexpr dynamic_content(const tao::tuple<Ts...>& entities) :
        _type(ctti::type_id<tao::tuple<Ts...>>())
    {
        // apply ->ticket() to tuple
        _data = reinterpret_cast<uintptr_t*>(new tickets_tuple_t<Ts...>(tao::apply([](auto... entities) {
            return tao::tuple(entities->ticket()...);
        }, entities)));
    }

    explicit constexpr dynamic_content(dynamic_content&& other) noexcept = default;
    constexpr dynamic_content& operator=(dynamic_content&& other) noexcept = default;

    constexpr auto clone() const
    {
        return dynamic_content(*this);
    }
    
    template <typename... Ts>
    constexpr inline auto data()
    {
        static_assert(ctti::type_id<tao::tuple<Ts...>>() == _type, "Mismatch in types");
        assert(ctti::type_id<tao::tuple<Ts...>>() == _type && "Mismatch in types");

        return reinterpret_cast<tickets_tuple_t<Ts...>*>(_data);
    }

    template <typename T, typename... Ts>
    constexpr inline T* get()
    {
        auto tuple = data<Ts...>();
        auto ticket = std::get<ticket_of<T>>(*tuple);

        if (ticket->valid())
        {
            return ticket->get()->derived();
        }

        return nullptr;
    }

protected:
    explicit constexpr dynamic_content(const dynamic_content& other) = default;

private:
    ctti::type_id_t _type;
    uintptr_t* _data;
};
