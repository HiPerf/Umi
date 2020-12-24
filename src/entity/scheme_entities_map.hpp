#pragma once

#include "containers/ticket.hpp"
#include "containers/unsafe_ticket_ref.hpp"
#include "common/tao.hpp"

#include <ctti/type_id.hpp>
#include <inttypes.h>


template <typename D> class entity;


// TODO(gpascualg): Use something along the lines of https://github.com/serge-sans-paille/frozen but with dynamic construction size?

class scheme_entities_map
{
public:
    template <typename T> using bare_t = typename std::remove_pointer_t<std::decay_t<T>>;

    scheme_entities_map() = default;
    scheme_entities_map(scheme_entities_map&& other) noexcept = default;
    scheme_entities_map& operator=(scheme_entities_map&& other) noexcept = default;

    template <typename... Ts>
    scheme_entities_map(const tao::tuple<Ts...>& entities)
    {
        tao::apply([this](auto... entities) {
            (..., _entities.emplace(
                ctti::type_id<bare_t<decltype(entities)>>().hash(), 
                unsafe_ticket_ref::from<bare_t<Ts>>(entities->ticket()))
            );
        }, entities);
    }

    inline auto clone() const
    {
        return scheme_entities_map(*this);
    }

    template <typename T>
    inline T* get() const
    {
        if (auto it = _entities.find(ctti::type_id<T>().hash()); it != _entities.end())
        {
            return it->second.template get<T>();
        }

        return nullptr;
    }

    template <typename T>
    inline void push(entity<T>* entity)
    {
        // Search all entities (including this one) and push in their map
        for (auto& [hash, ticket] : _entities)
        {
            if (auto other = ticket.template get<T>())
            {
                other->_entities._entities.emplace(ctti::type_id<bare_t<T>>().hash(), unsafe_ticket_ref::from<bare_t<T>>(entity->ticket()));
                entity->_entities._entities.emplace(hash, ticket);
            }
        }
    }

protected:
    explicit scheme_entities_map(const scheme_entities_map& other) = default;

private:
    std::unordered_map<ctti::detail::hash_t, unsafe_ticket_ref> _entities;
};
