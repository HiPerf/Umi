#pragma once

#include "maps/cell.hpp"
#include "maps/region.hpp"

#include <entity/entity.hpp>


class map : public entity<map>
{
public:
    void on_moved(transform* transform);

private:
    void check_region(map_aware* who, transform* transform, const glm::vec3& position);
    void check_cell(map_aware* who, transform* transform, const glm::vec3& position);

    region* get_or_create_region(const region::offset_t& offset);

private:
    std::unordered_map<typename cell::offset_t::hash_t, cell*> _cells;
    std::unordered_map<typename region::offset_t::hash_t, region*> _regions;
};
