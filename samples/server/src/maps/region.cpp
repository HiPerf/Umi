#include "maps/region.hpp"


region::region(const offset_t& offset) :
    _offset(offset),
    _store(),
    _map_aware_scheme(_store)
{}

void region::move_from_region(region* other, map_aware* who, transform* trf)
{
    other->_map_aware_scheme.move(_map_aware_scheme, who, trf);

    // TODO(gpascualg): Helper method for this
    other->_map_aware_scheme.get<map_aware>().move(who->ticket(), _map_aware_scheme.get<map_aware>());
    other->_map_aware_scheme.get<transform>().move(trf->ticket(), _map_aware_scheme.get<transform>());
}
