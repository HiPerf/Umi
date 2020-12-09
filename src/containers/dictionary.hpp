#pragma once

#include "common/types.hpp"
#include "containers/pooled_static_vector.hpp"

#include <unordered_map>



template <typename T, typename B, uint16_t InitialSize>
class dictionary : public pooled_static_vector<T, B, InitialSize, dictionary<T, B, InitialSize>>
{
    friend class pooled_static_vector<T, B, InitialSize, dictionary>;
    template <typename... D> friend class scheme;

public:
    using base_t = B;
    using derived_t = T;

public:
    dictionary();

    inline typename ticket<B>::ptr get(entity_id_t id);
    inline derived_t* get_derived_or_null(entity_id_t id);

    template <uint16_t OtherSize>
    B* move(entity_id_t id, dictionary<T, B, OtherSize>& to);

    template <uint16_t OtherSize>
    B* move(const typename ticket<B>::ptr ticket, dictionary<T, B, OtherSize>& to);

protected:
    void register_alloc(entity_id_t id, T* object);
    void register_free(entity_id_t id);
    void clear();

protected:
    std::unordered_map<entity_id_t, typename ticket<B>::ptr> _tickets;
};


template <typename T, typename B, uint16_t InitialSize>
dictionary<T, B, InitialSize>::dictionary() :
    pooled_static_vector<T, B, InitialSize, dictionary<T, B, InitialSize>>()
{}

template <typename T, typename B, uint16_t InitialSize>
inline typename ticket<B>::ptr dictionary<T, B, InitialSize>::get(entity_id_t id)
{
    if (auto it = _tickets.find(id); it != _tickets.end())
    {
        return it->second;
    }
    return nullptr;
}

template <typename T, typename B, uint16_t InitialSize>
inline typename dictionary<T, B, InitialSize>::derived_t* dictionary<T, B, InitialSize>::get_derived_or_null(entity_id_t id)
{
    if (auto it = _tickets.find(id); it != _tickets.end())
    {
        return it->second->template get<B>()->derived();
    }
    return nullptr;
}

template <typename T, typename B, uint16_t InitialSize>
template <uint16_t OtherSize>
B* dictionary<T, B, InitialSize>::move(entity_id_t id, dictionary<T, B, OtherSize>& to)
{
    return move(get(id), to);
}

template <typename T, typename B, uint16_t InitialSize>
template <uint16_t OtherSize>
B* dictionary<T, B, InitialSize>::move(const typename ticket<B>::ptr ticket, dictionary<T, B, OtherSize>& to)
{
    return pooled_static_vector<T, B, InitialSize, dictionary<T, B, InitialSize>>::move_impl(
        reinterpret_cast<T*>(ticket->get()), to);
}

template <typename T, typename B, uint16_t InitialSize>
void dictionary<T, B, InitialSize>::register_alloc(entity_id_t id, T* object)
{
    _tickets.emplace(id, object->ticket());
}

template <typename T, typename B, uint16_t InitialSize>
void dictionary<T, B, InitialSize>::register_free(entity_id_t id)
{
    _tickets.erase(id);
}

template <typename T, typename B, uint16_t InitialSize>
void dictionary<T, B, InitialSize>::clear()
{
    _tickets.clear();
    pooled_static_vector<T, B, InitialSize, dictionary<T, B, InitialSize>>::clear();
}
