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

class transform;

class client : public entity<client>,
    public kaminari::client<
        kumo::protocol_queues<
            test_allocator<kaminari::immediate_packer_allocator_t>,
            test_allocator<kaminari::immediate_packer_allocator_t>,
            test_allocator<kaminari::ordered_packer_allocator_t>
        >
    >
{
    struct database_data
    {
        std::string username;
    };

public:
    client();

    void construct(const udp::endpoint& endpoint);
    void update(update_inputs_t, const base_time& diff);
    void update(update_outputs_t, const base_time& diff);

    inline const udp::endpoint& endpoint() const;
    inline ingame_status ingame_status() const;
    inline const std::optional<database_data>& database_information() const;
    inline transform* ingame_entity() const;

    inline void database_information(database_data&& data);
    inline void ingame_entity(transform* entity);

    inline void handshake_done();
    inline void login_pending();
    inline void login_done();
    inline void character_selected();

private:
    udp::endpoint _endpoint;
    enum ingame_status _ingame_status;
    std::optional<database_data> _database_information;
    transform* _ingame_entity;
};


inline const udp::endpoint& client::endpoint() const
{
    return _endpoint;
}

inline ingame_status client::ingame_status() const
{
    return _ingame_status;
}

inline const std::optional<client::database_data>& client::database_information() const
{
    return _database_information;
}

inline transform* client::ingame_entity() const
{
    return _ingame_entity;
}

inline void client::database_information(database_data&& data)
{
    _database_information.emplace(std::move(data));
}

inline void client::ingame_entity(transform* entity)
{
    _ingame_entity = entity;
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

inline void client::character_selected()
{
    _ingame_status = ingame_status::in_world;
}
