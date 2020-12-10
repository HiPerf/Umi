#pragma once

#include "common/types.hpp"
#include "containers/ticket.hpp"

#include <unordered_map>


template <typename T, typename B>
class static_store
{
public:
    static inline typename ticket<B>::ptr get(entity_id_t id);
    static inline T* get_derived_or_null(entity_id_t id);

protected:
    void clear();

protected:
    static inline std::unordered_map<entity_id_t, typename ticket<B>::ptr> _tickets;
};


template <typename T, typename B>
inline typename ticket<B>::ptr static_store<T, B>::get(entity_id_t id)
{
    if (auto it = _tickets.find(id); it != _tickets.end())
    {
        return it->second;
    }
    return nullptr;
}

template <typename T, typename B>
inline typename T* static_store<T, B>::get_derived_or_null(entity_id_t id)
{
    if (auto it = _tickets.find(id); it != _tickets.end())
    {
        return it->second->template get<B>()->derived();
    }
    return nullptr;
}

template <typename T, typename B>
void static_store<T, B>::clear()
{
    _tickets.clear();
}


