#pragma once

#include <containers/ticket.hpp>
#include <entity/entity.hpp>

#include "core/client.hpp"
#include "maps/offset.hpp"

#include <kaminari/broadcaster.hpp>


class map;
class transform;


class cell : public kaminari::broadcaster<cell>
{
    friend class map;

public:
    using offset_t = offset<float, 15>;

    cell(map* map, const offset_t& offset);

    inline const offset_t& offset() const;

    void entity_spawn(transform* transform, const glm::vec3& position);
    void move_to(cell* other, transform* transform, const glm::vec3& position);

    template <typename C>
    void broadcast(C&& callback);

    template <typename C>
    void broadcast_single(C&& callback);

private:
    // TODO(gpascualg): Auxiliary method to get client without including server.hpp here in a .hpp
    client* get_client(transform* transform);

private:
    map* _map;
    offset_t _offset;
    std::vector<typename ticket<entity<transform>>::ptr> _transforms;
};


inline const cell::offset_t& cell::offset() const
{
    return _offset;
}

template <typename C>
void cell::broadcast(C&& callback)
{
    broadcast_single(callback);

    for (auto offset : neighbours_of(_offset))
    {
        // HACK(gpascualg): This template uses an incomplete map* type
        if (auto cell = _map->get_cell(offset))
        {
            broadcast_single(callback);
        }
    }
}

template <typename C>
void cell::broadcast_single(C&& callback)
{
    for (auto& ticket : _transforms)
    {
        if (auto transform = ticket->get()->derived())
        {
            if (auto client = get_client(transform))
            {
                callback(client->super_packet());
            }
        }
    }
}
