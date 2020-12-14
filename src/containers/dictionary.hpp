#pragma once

#include "common/types.hpp"
#include "containers/store.hpp"
#include "containers/static_store.hpp"
#include "containers/pooled_static_vector.hpp"

#include <unordered_map>


template <typename T, typename B, uint16_t InitialSize, typename ST = store<T, B>>
class dictionary : public pooled_static_vector<T, B, InitialSize, dictionary<T, B, InitialSize, ST>>, public ST
{
    friend class pooled_static_vector<T, B, InitialSize, dictionary>;
    template <typename... D> friend class scheme;

public:
    using base_t = B;
    using derived_t = T;

public:
    dictionary();

    template <uint16_t OtherSize, typename OtherST>
    B* move(entity_id_t id, dictionary<T, B, OtherSize, OtherST>& to);

    template <uint16_t OtherSize, typename OtherST>
    B* move(const typename ticket<B>::ptr ticket, dictionary<T, B, OtherSize, OtherST>& to);

    void clear();

protected:
    void register_alloc(entity_id_t id, T* object);
    void register_free(entity_id_t id);
};


template <typename T, typename B, uint16_t InitialSize, typename ST>
dictionary<T, B, InitialSize, ST>::dictionary() :
    pooled_static_vector<T, B, InitialSize, dictionary<T, B, InitialSize, ST>>()
{}

template <typename T, typename B, uint16_t InitialSize, typename ST>
template <uint16_t OtherSize, typename OtherST>
B* dictionary<T, B, InitialSize, ST>::move(entity_id_t id, dictionary<T, B, OtherSize, OtherST>& to)
{
    if (auto ticket = ST::get(id); ticket && ticket->valid())
    {
        return move(ticket, to);
    }
}

template <typename T, typename B, uint16_t InitialSize, typename ST>
template <uint16_t OtherSize, typename OtherST>
B* dictionary<T, B, InitialSize, ST>::move(const typename ticket<B>::ptr ticket, dictionary<T, B, OtherSize, OtherST>& to)
{
    return pooled_static_vector<T, B, InitialSize, dictionary<T, B, InitialSize, ST>>::move_impl(
        reinterpret_cast<T*>(ticket->get()), to);
}

template <typename T, typename B, uint16_t InitialSize, typename ST>
void dictionary<T, B, InitialSize, ST>::register_alloc(entity_id_t id, T* object)
{
    ST::_tickets.emplace(id, object->ticket());
}

template <typename T, typename B, uint16_t InitialSize, typename ST>
void dictionary<T, B, InitialSize, ST>::register_free(entity_id_t id)
{
    ST::_tickets.erase(id);
}

template <typename T, typename B, uint16_t InitialSize, typename ST>
void dictionary<T, B, InitialSize, ST>::clear()
{
    ST::clear();
    pooled_static_vector<T, B, InitialSize, dictionary<T, B, InitialSize>>::clear();
}
