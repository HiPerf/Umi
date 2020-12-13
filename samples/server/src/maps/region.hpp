#pragma once

#include <containers/ticket.hpp>
#include <entity/entity.hpp>
#include <entity/scheme.hpp>

#include "maps/offset.hpp"
#include "entities/map_aware.hpp"
#include "entities/transform.hpp"

class transform;


class region
{
    friend class map;

public:
    constexpr static inline uint16_t static_alloc_size = 150;
    using offset_t = offset<float, 150>;
    template <typename T> using dic_t = dictionary<T, entity<T>, static_alloc_size>;

    region(const offset_t& offset);

    inline const offset_t& offset() const;

protected:
    void move_from_region(region* other, map_aware* who, transform* trf);

private:
    offset_t _offset;

    scheme_store<dic_t<map_aware>, dic_t<transform>> _store;
    decltype(scheme_maker<map_aware, transform>()(_store)) _map_aware_scheme;
};


inline const region::offset_t& region::offset() const
{
    return _offset;
}
