#pragma once

#include "common/definitions.hpp"

#include <kumo/opcodes.hpp>
#include <kumo/structs.hpp>
#include <kaminari/client/basic_client.hpp>


class handler
{
protected:
    static bool handle_client_error(::kaminari::basic_client* kumo_client, ::kumo::opcode opcode);
    static bool check_client_status(::kaminari::basic_client* kumo_client, ingame_status status);
    static bool on_move(::kaminari::basic_client* kumo_client, const ::kumo::movement& data, uint64_t timestamp);
    static bool on_handshake(::kaminari::basic_client* kumo_client, const ::kumo::client_handshake& data, uint64_t timestamp);
};
