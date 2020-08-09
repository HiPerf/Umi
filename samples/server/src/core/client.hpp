#pragma once

#include "common/definitions.hpp"
#include <entity/entity.hpp>

#include <kaminari/client/client.hpp>
#include <kaminari/super_packet_reader.hpp>
#include <kumo/protocol_queues.hpp>

#include <boost/asio.hpp>


template <typename T>
class test_allocator : public std::allocator<T>
{
public:
    test_allocator(int x) :
        std::allocator<T>()
    {}
};


using udp = boost::asio::ip::udp;

class client : public entity<client>, 
    public kaminari::client<
        kumo::protocol_queues<
            test_allocator<kaminari::immediate_packer_allocator_t>, 
            test_allocator<kaminari::ordered_packer_allocator_t>
        >
    >
{
public:
    client();
    
    void construct(const udp::endpoint& endpoint);
    void update(const base_time& diff);

    void push_data(udp_buffer* buffer, uint16_t size);

private:
    udp::endpoint _endpoint;
};
