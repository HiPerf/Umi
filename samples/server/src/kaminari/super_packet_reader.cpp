#include "network/super_packet_reader.hpp"
#include "network/protocol.hpp"
#include "network/buffers/packet_reader.hpp"
#include "network/autogen/marshal.hpp"
#include "cx/overflow.hpp"
#include "debug/debug.hpp"
#include "debug/profiler.hpp"
#include "server/server.hpp"


SuperPacketReader::~SuperPacketReader()
{}

void SuperPacketReader::handle_packets(Client* client, Protocol* protocol)
{
    auto guard = profiler::instance->protocol_handle_packets.measure()
        .with("id", client->id());

    // Start reading old blocks
    uint8_t num_blocks = *reinterpret_cast<const uint8_t*>(_ack_end);
    const uint8_t* block_pos = _ack_end + sizeof(uint8_t);

    // Set some upper limit to avoid exploits
    int remaining = 500 - (block_pos - _data); // Keep it signed on purpouse
    for (uint8_t i = 0; i < num_blocks; ++i)
    {
        uint16_t block_id = *reinterpret_cast<const uint16_t*>(block_pos);
        uint8_t num_packets = *reinterpret_cast<const uint8_t*>(block_pos + sizeof(uint16_t));
        if (num_packets == 0)
        {
            // TODO(gpascualg): Should we kick the player for packet forging?
            return;
        }

        const TimePoint block_timestamp = protocol->block_timestamp(block_id);
        block_pos += sizeof(uint16_t) + sizeof(uint8_t);
        remaining -= sizeof(uint16_t) + sizeof(uint8_t);

        for (uint8_t j = 0; j < num_packets && remaining > 0; ++j)
        {
            PacketReader reader(block_pos, block_timestamp);
            uint16_t length = reader.length();
            block_pos += length;
            remaining -= length;

            if (length < Packet::DataStart || remaining < 0)
            {
                // TODO(gpascualg): Should we kick the player for packet forging?
                return;
            }   

            if (protocol->resolve(&reader, block_id))
            {
                rpc::marshal::handle_packet(&reader, client);
            }
        }
    }
}
