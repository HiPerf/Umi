#include "core/client.hpp"

#include <iostream>


client::client() :
    kaminari::client<
        kumo::protocol_queues<
            test_allocator<kaminari::immediate_packer_allocator_t>, 
            test_allocator<kaminari::ordered_packer_allocator_t>
        >
    >(5, 9, 9, 99)
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

void client::push_data(udp_buffer* buffer, uint16_t size)
{
    
}
