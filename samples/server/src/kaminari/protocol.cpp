#include "debug/profiler.hpp"
#include "network/autogen/rpc.hpp"
#include "network/protocol.hpp"
#include "network/buffers/packet.hpp"
#include "network/buffers/packet_reader.hpp"
#include "network/super_packet.hpp"
#include "network/super_packet_reader.hpp"
#include "server/client.hpp"
#include "server/server.hpp"


Protocol::Protocol()
{
    reset();
}

void Protocol::reset()
{
    _buffer_mode = BufferMode::NO_BUFFER;
    _since_last_send = 0;
    _since_last_recv = 0;
    _last_block_id_read = 0;
    _expected_block_id = 0;
    _timestamp = TimePoint(TimeBase(0));
    _timestamp_block_id = 0;
    _already_resolved.clear();
}

void Protocol::update(Client* client, SuperPacket* superpacket)
{
    auto guard = profiler::instance->protocol_update.measure();

    bool needs_ping = ++_since_last_send >= PingInterval;
    if (needs_ping)
    {
        _since_last_send = 0;
        auto timestamp = ::now();
        rpc::ping(client, { .timestamp = static_cast<uint64_t>(timestamp.time_since_epoch().count()) }, [timestamp, ticket = client->ticket()]() {
            if (!ticket->valid())
            {
                return;
            }

            // Now update the client lag as a running mean, divide by two to count only half round trip
            auto cl = ticket->get();
            auto elapsed = std::chrono::duration_cast<TimeBase>(::now() - timestamp);
            auto ms = elapsed.count();
            cl->lag(static_cast<uint64_t>(
                static_cast<float>(cl->lag()) * 0.9 +
                static_cast<float>(ms) / 2 * 0.1)
            );
            log<LOG_LEVEL_DEBUG, LOG_CLIENT>(std::forward_as_tuple("Client updated lag estimation is ", cl->lag()));
        });
    }

    if (superpacket->finish())
    {
        gServer->send(client, *superpacket);
    }
}

void Protocol::read(Client* client)
{
    auto guard = profiler::instance->protocol_read.measure()
        .with("id", client->id());

    if (_buffer_mode == BufferMode::BUFFERING)
    {
        // TODO(gpascualg): Implement packet buffering
        assert_true(false, "BufferMode::BUFFERING not yet supported");
    }

    if (client->_pending_superpackets.empty())
    {
        if (++_since_last_recv > MaxBlocksUntilDisconnection)
        {
            gServer->disconnect(client);
        }

        return;
    }

    SuperPacketReader* reader = nullptr;
    if (_buffer_mode == BufferMode::NO_BUFFER)
    {
        reader = client->_pending_superpackets.front();
    }
    else if (_buffer_mode == BufferMode::READY)
    {
        // TODO(gpascualg): Reading from the buffered packets, respecting order
        assert_true(false, "BufferMode::READY not yet supported");
    }

    read_impl(client, reader, &client->_superpacket);

    client->_pending_superpackets.pop_front();
    gServer->coro_pool<SuperPacketReader>::free(reader);
}

void Protocol::read_impl(Client* client, SuperPacketReader* reader, SuperPacket* superpacket)
{
    auto guard = profiler::instance->protocol_read_impl.measure()
        .with("id", client->id());

    uint16_t current_id = reader->id();
    log<LOG_LEVEL_DEBUG, LOG_SUPERPACKET>(std::forward_as_tuple("Parsing SuperPacket[", current_id, 
        "] with length ", reader->length()));

    // Unordered packets are not to be parsed, as they contain outdated information
    // in case that information was important, it would have already been resent
    if (!is_expected(current_id))
    {
        log<LOG_LEVEL_DEBUG, LOG_SUPERPACKET>("Old packet discarded");
        // It is still a recv, though
        _since_last_recv = 0;
        return;
    }

    // Acknowledge user acks
    reader->iterate_acks([superpacket](auto ack)
    {
        superpacket->ack(ack);
    });

    // Let's add it to pending acks
    if (reader->has_data())
    {
        superpacket->schedule_ack(current_id);
    }

    // Update timestamp
    _timestamp_block_id = current_id;
    _timestamp = gServer->now();

    // Actually handle inner packets
    reader->handle_packets(client, this);

    // TODO(gpascualg): This can still be improved by much
    // Clear all resolved packets ranging from last received ID to current minus Resync threshold
    for (auto id = _last_block_id_read; cx::overflow::le(id, current_id);)
    {
        _already_resolved.erase(cx::overflow::sub(id, MaximumBlocksUntilResync));
        id = cx::overflow::inc(id);
    }

    _last_block_id_read = current_id;
    _expected_block_id = cx::overflow::inc(current_id);
    _since_last_recv = 0;
}

bool Protocol::resolve(PacketReader* packet, uint16_t block_id)
{
    auto guard = profiler::instance->protocol_resolve.measure()
        .with("id", block_id);

    auto opcode = packet->opcode();
    uint8_t id = packet->id();

    if (auto it = _already_resolved.find(block_id); it != _already_resolved.end())
    {
        // That block has already been parsed
        auto& parsed_deps = it->second;
        if (auto jt = parsed_deps.find(id); jt != parsed_deps.end())
        {
            return false;
        }

        parsed_deps.insert(id);
    }
    else
    {
        _already_resolved.emplace(block_id, std::set<uint8_t> { id });
    }
    
    return true;
}
