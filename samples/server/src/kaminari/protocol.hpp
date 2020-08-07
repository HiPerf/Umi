#pragma once

#include "cx/overflow.hpp"
#include "common/switches.hpp"
#include "network/autogen/opcodes.hpp"
#include "network/buffers/packet.hpp"

#include <unordered_map>
#include <set>

#include <boost/pool/pool.hpp>


class Client;
class PacketReader;
class SuperPacket;
class SuperPacketReader;

enum class BufferMode
{
    NO_BUFFER,
    BUFFERING,
    READY
};

enum class PacketQueue
{
    UNRELIABLE,
    RELIABLE,
    EVENTUALLY_SYNCED,
    ORDERED // TODO(gpascualg): Multiple ordered queues?
};

class Protocol
{
public:
    Protocol();

    void reset();
    void update(Client* client, SuperPacket* superpacket);
    void read(Client* client);
    bool resolve(PacketReader* packet, uint16_t block_id);

    inline uint16_t last_block_id_read() const;
    inline uint16_t expected_block_id() const;
    inline bool is_expected(uint16_t id) const;

    inline void set_timestamp(TimePoint timestamp, uint16_t block_id);
    inline TimePoint block_timestamp(uint16_t block_id);

private:
    void read_impl(Client* client, SuperPacketReader* reader, SuperPacket* superpacket);

private:
    BufferMode _buffer_mode;
    uint16_t _since_last_send;
    uint16_t _since_last_recv;
    uint16_t _last_block_id_read;
    uint16_t _expected_block_id;
    TimePoint _timestamp;
    uint16_t _timestamp_block_id;
    std::unordered_map<uint16_t, std::set<uint8_t>> _already_resolved;
};


inline uint16_t Protocol::last_block_id_read() const 
{ 
    return _last_block_id_read; 
}

inline uint16_t Protocol::expected_block_id() const
{ 
    return _expected_block_id; 
}

inline bool Protocol::is_expected(uint16_t id) const
{
    return _expected_block_id == 0 || !cx::overflow::le(id, _expected_block_id); 
}

inline void Protocol::set_timestamp(TimePoint timestamp, uint16_t block_id)
{
    _timestamp = timestamp;
    _timestamp_block_id = block_id;
}
    
inline TimePoint Protocol::block_timestamp(uint16_t block_id)
{
    if (block_id >= _timestamp_block_id)
    {
        return _timestamp + (block_id - _timestamp_block_id) * WorldHeartBeat;
    }

    return _timestamp - (_timestamp_block_id - block_id) * WorldHeartBeat;
}
