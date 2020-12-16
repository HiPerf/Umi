#include "core/server.hpp"
#include "maps/region.hpp"
#include "maps/map.hpp"

region::region(map* map, const offset_t& offset) :
    _map(map),
    _offset(offset),
    _common_store(),
    _moving_store(),
    _map_aware_scheme(_common_store),
    _moving_transforms_scheme(_moving_store),
    _transforms_updater(tao::make_tuple(&_moving_transforms_scheme.get<transform>()))
{}

void region::move_to(region* other, map_aware* who, transform* trf)
{
    // If it is moving, it means it's on the moving scheme
    _moving_transforms_scheme.move(other->_moving_transforms_scheme, who, trf);
}

void region::remove_entity(transform* transform)
{
    if (transform->is_moving())
    {
        _moving_transforms_scheme.free(transform->get<map_aware>());
        _moving_transforms_scheme.free(transform);
    }
    else
    {
        _map_aware_scheme.free(transform->get<map_aware>());
        _map_aware_scheme.free(transform);
    }
}

void region::on_entity_created(map_aware* map_aware, transform* transform, const glm::vec3& position)
{
    // Push initial position
    transform->push(server::instance->now(), position, glm::vec3{ 1, 0, 0 }, 0.0);

    // Create transactional object and push it to the entity (so that we can do transform->get<transaction>())
    auto transaction = server::instance->create_entity_transaction(map_aware->db_id());
    transform->push_component(transaction);
}
