#include "core/server.hpp"
#include "maps/map.hpp"

#include <updater/executor_registry.hpp>


void map::on_moved(transform* transform)
{
    auto position = transform->position(server::instance->now());
    //auto who = transform->scheme<decltype(region::_map_aware_scheme)>()->get<map_aware>().get_derived_or_null(transform->id());
    auto who = transform->get<map_aware>();
    
    check_region(who, transform, position);
    check_cell(who, transform, position);
}

void map::check_region(map_aware* who, transform* transform, const glm::vec3& position)
{
    auto current_offset = who->current_region()->offset();
    auto new_offset = region::offset_t::of(position.x, position.z);

    if (current_offset != new_offset)
    {
        executor_registry::current()->schedule_if([this, current=who->current_region(), new_offset](auto who, auto transform)
        {
            // Move entity to the new region
            get_or_create_region(new_offset)->move_from_region(current, who, transform);

        }, who->ticket(), transform->ticket());
    }
}

void map::check_cell(map_aware* who, transform* transform, const glm::vec3& position)
{

}

region* map::get_or_create_region(const region::offset_t& offset)
{
    if (auto it = _regions.find(offset.hash()); it != _regions.end())
    {
        return it->second;
    }

    return _regions.emplace(offset.hash(), new region(offset)).first->second;
}
