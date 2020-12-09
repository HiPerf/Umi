#include "common/definitions.hpp"
#include "core/client.hpp"
#include "core/server.hpp"

#include <iostream>


client::client() :
    kaminari::client<
        kumo::protocol_queues<
            test_allocator<kaminari::immediate_packer_allocator_t>, 
            test_allocator<kaminari::ordered_packer_allocator_t>
        >
    >(5, test_allocator<kaminari::immediate_packer_allocator_t>(2), test_allocator<kaminari::ordered_packer_allocator_t>(5))
{}

void client::construct(const udp::endpoint& endpoint)
{
    _endpoint = endpoint;
    _ingame_status = ingame_status::new_connection;

    reset();
}

void client::update(update_inputs_t, const base_time& diff)
{
    if (pending_disconnection())
    {
        // Check if it needs disconnection now
        if (connexion_status() == basic_client::status::PENDING_DISCONNECTION)
        {
            server::instance->disconnect_client(this);
        }

        // Do nothing if it is pending disconnection
        return;
    }

    // Read inputs
    _protocol.read<kumo::marshal, base_time>(this, super_packet());
}

void client::update(update_outputs_t, const base_time& diff)
{
    // Do nothing if it is pending disconnection
    if (pending_disconnection())
    {
        return;
    }

    // Send packet if any
    if (_protocol.update(this, super_packet()))
    {
        server::instance->send_client_outputs(this);
    }
}

