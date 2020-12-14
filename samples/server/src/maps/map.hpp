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

    void create_entity_at(const glm::vec3& position);

private:
    std::unordered_map<typename cell::offset_t::hash_t, cell*> _cells;
    std::unordered_map<typename region::offset_t::hash_t, region*> _regions;
};
