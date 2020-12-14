#include "core/server.hpp"
#include "maps/region.hpp"
#include "maps/map.hpp"

region::region(const offset_t& offset) :
    _offset(offset),
    _common_store(),
    _moving_store(),
    _map_aware_scheme(_common_store),
    _moving_transforms_scheme(_moving_store),
    _transforms_updater(tao::make_tuple(&_moving_transforms_scheme.get<transform>()))
{}

void region::create_entity(map* map, cell* cell, const glm::vec3& position)
{
    server::instance->create_with_callback(
        _map_aware_scheme,
        [position](auto map_aware, auto transform) {
            transform->push(server::instance->now(), position, glm::vec3{ 1, 0, 0 }, 0.0);
            return tao::tuple(map_aware, transform);
        },
        _map_aware_scheme.args<map_aware>(),
        _map_aware_scheme.args<transform>(map, this, cell)
    );
}

void region::move_to(region* other, map_aware* who, transform* trf)
{
    // If it is moving, it means it's on the moving scheme
    _moving_transforms_scheme.move(other->_moving_transforms_scheme, who, trf);
}
