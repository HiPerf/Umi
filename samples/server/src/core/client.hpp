#pragma once

#include "common/definitions.hpp"
#include <entity/entity.hpp>

#include <kaminari/super_packet.hpp>
#include <kumo/protocol_queues.hpp>

#include <boost/asio.hpp>


using udp = boost::asio::ip::udp;

class client : public entity<client>
{
public:
    void construct(const udp::endpoint& endpoint);
    void update(const base_time& diff);

    void push_data(udp_buffer* buffer, uint16_t size);

private:
    udp::endpoint _endpoint;
    kaminari::super_packet<kumo::protocol_queues> _super_packet;
};
