#pragma once

#include "network/super_packet.hpp"

#include <inttypes.h>

class Client;
class Protocol;

class SuperPacketReader
{
public:
    explicit constexpr SuperPacketReader();
    SuperPacketReader(const SuperPacketReader&) = delete; // Do not copy!
    SuperPacketReader(SuperPacketReader&&) = default;
    ~SuperPacketReader();

    inline uint16_t length() const;
    inline uint16_t id() const;

    template <typename C>
    void iterate_acks(C&& callback);

    inline uint8_t* data();
    inline bool has_data();

    void handle_packets(Client* client, Protocol* protocol);
private:
    uint8_t _data[SuperPacket::MaxSize];
    const uint8_t* _ack_end;
};


constexpr SuperPacketReader::SuperPacketReader() :
    _data(),
    _ack_end(nullptr)
{}

inline uint16_t SuperPacketReader::length() const
{
    return *reinterpret_cast<const uint16_t*>(_data);
}

inline uint16_t SuperPacketReader::id() const
{
    return *reinterpret_cast<const uint16_t*>(_data + sizeof(uint16_t));
}

template <typename C>
void SuperPacketReader::iterate_acks(C&& callback)
{
    _ack_end = _data + sizeof(uint16_t) * 2;
    uint8_t num_acks = *reinterpret_cast<const uint8_t*>(_ack_end);
    _ack_end += sizeof(uint8_t);

    for (uint8_t i = 0; i < num_acks; ++i)
    {
        uint16_t ack = *reinterpret_cast<const uint16_t*>(_ack_end);
        callback(ack);
        _ack_end += sizeof(uint16_t);
    }
}

inline uint8_t* SuperPacketReader::data()
{
    return _data;
}

inline bool SuperPacketReader::has_data()
{
    return *reinterpret_cast<const uint8_t*>(_ack_end) != 0x0;
}
