#pragma once
#include <optional>
#include <vector>
#include <inttypes.h>
namespace kumo
{
    struct client_handshake;
    struct status;
    struct complex;
    struct has_id;
    struct spawn_data;
    struct movement;
}

namespace kumo
{
    struct client_handshake
    {
    public:
        uint32_t version;
    };

    struct status
    {
    public:
        bool success;
    };

    struct complex
    {
    public:
        std::optional<uint32_t> x;
        std::vector<spawn_data> y;
        int32_t z;
        std::optional<std::vector<bool>> w;
    };

    struct has_id
    {
    public:
        int64_t id;
    };

    struct spawn_data
    {
    public:
        int64_t id;
        int8_t x;
        int8_t y;
    };

    struct movement
    {
    public:
        int8_t direction;
    };

}
