#pragma once

#include "common/definitions.hpp"
#include <entity/entity.hpp>

#include <kaminari/client/client.hpp>
#include <kaminari/super_packet_reader.hpp>
#include <kaminari/types/data_wrapper.hpp>
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


struct update_inputs_t { explicit update_inputs_t() = default; };
struct update_outputs_t { explicit update_outputs_t() = default; };

inline constexpr update_inputs_t update_inputs{};
inline constexpr update_outputs_t update_outputs{};

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
    void update(update_inputs_t, const base_time& diff);
    void update(update_outputs_t, const base_time& diff);

    inline const udp::endpoint& endpoint() const;
    inline ingame_status ingame_status() const;

    inline void handshake_done();
    inline void login_pending();
    inline void login_done();

private:
    udp::endpoint _endpoint;
    enum class ingame_status _ingame_status;
};


inline const udp::endpoint& client::endpoint() const
{
    return _endpoint;
}

inline ingame_status client::ingame_status() const
{
    return _ingame_status;
}

inline void client::handshake_done()
{
    _ingame_status = ingame_status::handshake_done;
}

inline void client::login_pending()
{
    _ingame_status = ingame_status::login_pending;
}

inline void client::login_done()
{
    _ingame_status = ingame_status::login_done;
}

