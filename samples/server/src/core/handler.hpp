#pragma once

#include <kumo/opcodes.hpp>
#include <kumo/structs.hpp>


namespace kaminari
{
    class client;
}

class handler
{
protected:
    static bool handle_client_error(::kaminari::client* client, ::kumo::opcode opcode)
    {
        return false;
    }

    static bool check_client_status(::kaminari::client* client, uint8_t status)
    {
        return true;
    }

    static bool on_move(::kaminari::client* client, const ::kumo::movement& data, uint64_t timestamp)
    {
        return true;
    }
};
