#include <kaminari/buffers/packet_reader.hpp>


namespace kaminari
{
    packet_reader::packet_reader(const uint8_t* data, uint64_t block_timestamp) :
        _data(data),
        _block_timestamp(block_timestamp)
    {
        _ptr = data + Packet::DataStart;
    }
}
