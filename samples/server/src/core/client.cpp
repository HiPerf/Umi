#include "core/client.hpp"

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

void client::update(const base_time& diff)
{
    if (!_protocol.read<kumo::marshal>(this, super_packet()))
    {
        // TODO(gpascualg): Disconnect client
    }
}
