#include "core/server.hpp"
#include "entities/transform.hpp"
#include "maps/cell.hpp"
#include "maps/map.hpp"
#include "maps/region.hpp"


transform::transform() :
    entity<transform>(),
    _buffer(16),
    _current_cell(nullptr),
    _current_region(nullptr),
    _is_moving(false)
{}

void transform::construct(map* map, region* region, cell* cell)
{
    _buffer.clear();
    _is_moving = false;
    _current_region = region;
    _current_cell = cell;
}

void transform::update(const base_time& diff, map* map)
{
    assert(_is_moving && "Only moving transforms need updates");
    
    auto position = this->position(server::instance->now());
        
    auto region_offset = _current_region->offset();
    auto new_region_offset = region::offset_t::of(position.x, position.z);

    if (region_offset != new_region_offset)
    {
        executor_registry::current()->schedule_if([this, map, new_region_offset](auto transform)
        {
            // Move entity to the new region
            auto new_region = map->get_or_create_region(new_region_offset);
            _current_region->move_to(new_region, get<map_aware>(), transform);
            _current_region = new_region;
        }, ticket());
    }

    auto cell_offset = _current_cell->offset();
    auto new_cell_offset = cell::offset_t::of(position.x, position.y);

    if (cell_offset != new_cell_offset)
    {
        executor_registry::current()->schedule_if([this, map, new_cell_offset](auto transform)
        {
            // Move entity to the new cell
            auto new_cell = map->get_or_create_cell(new_cell_offset);
            _current_cell->move_to(new_cell, transform);
            _current_cell = new_cell;
        }, ticket());
    }
}

void transform::push(const time_point_t& timestamp, const glm::vec3& position, const glm::vec3& forward, float speed)
{
    _buffer.push_back({
        .timestamp = timestamp,
        .position = position,
        .forward = forward,
        .speed = speed
    });
}
