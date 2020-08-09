#pragma once

#include <kumo/opcodes.hpp>
#include <kumo/structs.hpp>
#include <kaminari/client/basic_client.hpp>


class handler
{
protected:
    static bool handle_client_error(::kaminari::basic_client* client, ::kumo::opcode opcode)
    {
        return false;
    }

    static bool check_client_status(::kaminari::basic_client* client, uint8_t status)
    {
        return true;
    }

    static bool on_move(::kaminari::basic_client* client, const ::kumo::movement& data, uint64_t timestamp)
    {
        return true;
    }
};
