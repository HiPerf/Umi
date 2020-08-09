#pragma once

#include <inttypes.h>
#include <vector>

#include <boost/circular_buffer.hpp>


namespace kaminari
{
    class basic_client
    {
    public:
        basic_client();
        
        inline constexpr bool has_pending_super_packets() const;
        inline constexpr uint8_t* first_super_packet();

    private:
        boost::circular_buffer<uint8_t*> _pending_super_packets;
    };


    inline constexpr bool basic_client::has_pending_super_packets() const
    {
        return !_pending_super_packets.empty();
    }

    inline constexpr uint8_t* basic_client::first_super_packet()
    {
        auto data = _pending_super_packets.front();
        _pending_super_packets.pop_front();
        return data;
    }
}
