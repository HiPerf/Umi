#include "core/handler.hpp"
#include "core/server.hpp"

#include <kumo/config.hpp>


bool handler::handle_client_error(::kaminari::basic_client* kumo_client, ::kumo::opcode opcode)
{
    return false;
}

bool handler::check_client_status(::kaminari::basic_client* kumo_client, ingame_status status)
{
    auto client = (class client*)kumo_client;
    return client->ingame_status() == status;
}

bool handler::on_move(::kaminari::basic_client* kumo_client, const ::kumo::movement& data, uint64_t timestamp)
{
    return true;
}

bool handler::on_handshake(::kaminari::basic_client* kumo_client, const ::kumo::client_handshake& data, uint64_t timestamp)
{
    auto client = (class client*)kumo_client;

    bool matching_version = data.version != ::kumo::VERSION;
    kumo::send_handshake_response(client->super_packet(), { .success = matching_version });

    // Only accept latest protocol version
    if (matching_version)
    {
        server::instance->disconnect_client(client);
    }

    return true;
}
