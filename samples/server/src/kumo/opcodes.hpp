#pragma once
namespace kumo
{
    enum class opcode
    {
        handshake        = 0x047b,
        handshake_response = 0x7a01,
        do_sth           = 0x71bb,
        spawn            = 0x3858,
        move             = 0x15af,
    };
}
