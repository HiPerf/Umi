#include "core/server.hpp"
#include "maps/cell.hpp"
#include "entities/transform.hpp"


cell::cell(map* map, const offset_t& offset) :
    _map(map),
    _offset(offset)
{}

void entity_spawn(transform* transform, const glm::vec3& position)
{
    // This always happens sync
    kumo::broadcast_spawned_entity(transform->current_cell(), { .id = transform->id(), .x = position.x, .z = position.z });
}

void cell::move_to(cell* other, transform* transform, const glm::vec3& position)
{
    auto ticket = transform->ticket();
    _transforms.erase(std::find(_transforms.begin(), _transforms.end(), ticket)); // TODO(gpascualg): Optimize erase with a move
    other->_transforms.push_back(ticket);

    // Send despawns and spawns
    const int8_t dx = static_cast<int8_t>(other->offset().x() - offset().x());
    const int8_t dy = static_cast<int8_t>(other->offset().y() - offset().y());

    for (auto& offset : new_offsets_in_direction(other->offset(), dx, dy))
    {
        if (auto dest = _map->get_cell(offset))
        {
            kumo::broadcast_spawned_entity_single(dest, { .id = transform->id(), .x = position.x, .z = position.z });
        }
    }

    for (auto& offset : new_offsets_in_direction(offset(), -dx, -dy))
    {
        if (auto dest = _map->get_cell(offset); dest)
        {
            kumo::broadcast_despawned_entity_single(dest, { .id = transform->id() });
        }
    }
}

client* cell::get_client(transform* transform)
{
    return server::instance->get_client(transform->id());
}
