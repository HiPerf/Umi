#include "core/server.hpp"
#include "maps/map.hpp"

#include <updater/executor_registry.hpp>


void map::construct()
{
    for (auto& [ofs, cell] : _cells)
    {
        delete cell;
    }
    _cells.clear();

    for (auto& [ofs, region] : _regions)
    {
        delete region;
    }
    _regions.clear();
}

void map::update(const base_time& diff)
{
    // TODO(gpascualg): Can we make executors provide an easy way to do this?
    // All regions are updated in parallel
    boost::fibers::fiber([this, &diff]() mutable 
        {
            // 1. Schedule update all
            for (auto& [ofs, region] : _regions)
            {
                region->updater().update(std::ref(diff), this);
            }

            // 2. Schedule wait all
            for (auto& [ofs, region] : _regions)
            {
                region->updater().wait_update();
            }
        }).join();
}

region* map::get_region(const region::offset_t& offset) const
{
    if (auto it = _regions.find(offset.hash()); it != _regions.end())
    {
        return it->second;
    }

    return nullptr;
}

cell* map::get_cell(const cell::offset_t& offset) const
{
    if (auto it = _cells.find(offset.hash()); it != _cells.end())
    {
        return it->second;
    }

    return nullptr;
}

region* map::get_or_create_region(const region::offset_t& offset)
{
    if (auto region = get_region(offset))
    {
        return region;
    }

    auto region = _regions.emplace(offset.hash(), new ::region(this, offset)).first->second;
    return region;
}

cell* map::get_or_create_cell(const cell::offset_t& offset)
{
    if (auto cell = get_cell(offset))
    {
        return cell;
    }

    return _cells.emplace(offset.hash(), new cell(this, offset)).first->second;
}
