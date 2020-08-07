#pragma once

#include "network/autogen/queues.hpp"
#include "network/autogen/opcodes.hpp"

#include <inttypes.h>
#include <mutex>
#include <set>
#include <vector>
#include <unordered_map>

#include <boost/pool/pool.hpp>


class SuperPacket : public rpc::protocol_queues
{
public:
    constexpr static inline uint16_t MinHeaderSize = 5;
    constexpr static inline uint16_t MaxSize = 512;

    SuperPacket();
    SuperPacket(const SuperPacket& other) = delete;
    SuperPacket(SuperPacket&& other) = default;
    SuperPacket& operator=(SuperPacket&& other) = default;

    void reset();

    // Player has acked a packet
    void ack(uint16_t block_id);

    // We are ack'ing a player packet
    void schedule_ack(uint16_t block_id);

    // Obtain buffer
    bool finish();

    inline uint16_t id() const;
    inline const boost::asio::const_buffer& buffer() const;

private:
    uint16_t _id;

    // Needs acks from client
    std::vector<uint16_t> _pending_acks;
    
    // Data array
    uint8_t _data[MaxSize];
    boost::asio::const_buffer _buffer;
};


inline uint16_t SuperPacket::id() const
{
    return _id;
}

inline const boost::asio::const_buffer& SuperPacket::buffer() const
{
    return _buffer;    
}
