#pragma once

#include "maps/cell.hpp"

#include <entity/entity.hpp>


class map : public entity<map>
{

private:
    std::unordered_map<typename cell::offset_t::hash_t, cell*> _cells;
};
