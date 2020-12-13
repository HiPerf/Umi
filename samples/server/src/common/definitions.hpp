#pragma once

#include <inttypes.h>
#include <chrono>


using std_clock_t = std::chrono::steady_clock;
using sys_clock_t = std::chrono::system_clock;
using time_point_t = std_clock_t::time_point;
using base_time = std::chrono::milliseconds;

constexpr inline uint8_t TicksPerSecond = 20;
constexpr inline base_time HeartBeat = base_time(base_time(std::chrono::seconds(1)).count() / TicksPerSecond);

inline base_time elapsed(std_clock_t::time_point from, std_clock_t::time_point to)
{
    return std::chrono::duration_cast<base_time>(to - from);
}




// DATABASE
enum class database_collections : uint8_t
{
    accounts =          0,
    characters =        1
};






// CONSTANTS USED FOR FIBER MANAGEMENT
enum class FiberID
{
    ServerWorker =      0,  // This one CANNOT be changed, it is UMI internal
    NetworkWorker =     1,
    DatabaseWorker =    2
};







// COMMON STRUCTURES
struct udp_buffer
{
    uint8_t buffer[500]; // TODO(gpascualg): MaxSize
};





// OTHER
enum class ingame_status
{
    new_connection,
    handshake_done,
    login_pending,
    login_done,
    in_world,
};
