#pragma once

#include "common/definitions.hpp"

#include <kumo/opcodes.hpp>
#include <kumo/structs.hpp>
#include <kaminari/client/basic_client.hpp>


class handler
{
protected:
    static bool handle_client_error(::kaminari::basic_client* kaminari_client, ::kumo::opcode opcode);
    static bool check_client_status(::kaminari::basic_client* kaminari_client, ingame_status status);
    static bool on_move(::kaminari::basic_client* kaminari_client, const ::kumo::movement& data, uint64_t timestamp);
    static bool on_handshake(::kaminari::basic_client* kaminari_client, const ::kumo::client_handshake& data, uint64_t timestamp);
    static bool on_login(::kaminari::basic_client* kaminari_client, const ::kumo::login_data& data, uint64_t timestamp);
    static bool on_character_selected(::kaminari::basic_client* kaminari_client, const ::kumo::character_selection& data, uint64_t timestamp);
};
