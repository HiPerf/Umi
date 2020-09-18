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
}

void client::update(update_inputs_t, const base_time& diff)
{
    // Do nothing if it is pending disconnection
    if (connexion_status() == basic_client::status::PENDING_DISCONNECTION)
    {
        return;
    }

    // Read inputs
    _protocol.read<kumo::marshal, base_time>(this, super_packet());

    // Check if it needs disconnection now
    if (connexion_status() == basic_client::status::PENDING_DISCONNECTION)
    {
        server::instance->disconnect_client(this);
    }
}

void client::update(update_outputs_t, const base_time& diff)
{
    // Do nothing if it is pending disconnection
    if (connexion_status() == basic_client::status::PENDING_DISCONNECTION)
    {
        return;
    }

    // Send packet if any
    if (_protocol.update(this, super_packet()))
    {
        server::instance->send_client_outputs(this);
    }
}

