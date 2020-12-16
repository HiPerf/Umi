#pragma once

#include "maps/cell.hpp"
#include "maps/region.hpp"

#include <entity/entity.hpp>


class map : public entity<map>
{
public:
    map() = default;
    void construct();

    void update(const base_time& diff);

    region* get_region(const region::offset_t& offset) const;
    cell* get_cell(const cell::offset_t& offset) const;

    region* get_or_create_region(const region::offset_t& offset);
    cell* get_or_create_cell(const cell::offset_t& offset);

    template <typename C>
    void create_entity_at(uint64_t id, uint64_t db_id, const glm::vec3& position, C&& callback);

private:
    std::unordered_map<typename cell::offset_t::hash_t, cell*> _cells;
    std::unordered_map<typename region::offset_t::hash_t, region*> _regions;
};


template <typename C>
void map::create_entity_at(uint64_t id, uint64_t db_id, const glm::vec3& position, C&& callback)
{
    auto region = get_or_create_region(region::offset_t::of(position.x, position.z));
    region->create_entity(this, get_or_create_cell(cell::offset_t::of(position.x, position.y)), id, db_id, position, std::move(callback));
}
