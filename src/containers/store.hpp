#pragma once

#include "common/types.hpp"
#include "containers/ticket.hpp"

#include <unordered_map>


template <typename T, typename B>
class store
{
public:
    inline typename ticket<B>::ptr get(entity_id_t id) const;
    inline T* get_derived_or_null(entity_id_t id) const;

protected:
    void clear();

protected:
    std::unordered_map<entity_id_t, typename ticket<B>::ptr> _tickets;
};


template <typename T, typename B>
inline typename ticket<B>::ptr store<T, B>::get(entity_id_t id) const
{
    if (auto it = _tickets.find(id); it != _tickets.end())
    {
        return it->second;
    }
    return nullptr;
}

template <typename T, typename B>
inline T* store<T, B>::get_derived_or_null(entity_id_t id) const
{
    if (auto it = _tickets.find(id); it != _tickets.end())
    {
        return it->second->template get<B>()->derived();
    }
    return nullptr;
}

template <typename T, typename B>
void store<T, B>::clear()
{
    _tickets.clear();
}
