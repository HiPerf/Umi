#pragma once
#include <inttypes.h>
#include <boost/intrusive_ptr.hpp>
#include <kumo/structs.hpp>
#include <kaminari/buffers/packet.hpp>
#include <kaminari/buffers/packet_reader.hpp>
#include "core/handler.hpp"
namespace kumo
{
    class marshal;
}

namespace kumo
{
    class marshal:public handler
    {
    public:
        static bool unpack(::kaminari::buffers::packet_reader* packet, client_handshake& data);
        static uint8_t packet_size(const client_handshake& data);
        static uint8_t sizeof_client_handshake();
        static void pack(const boost::intrusive_ptr<::kaminari::buffers::packet>& packet, const status& data);
        static uint8_t packet_size(const status& data);
        static uint8_t sizeof_status();
        static bool unpack(::kaminari::buffers::packet_reader* packet, login_data& data);
        static uint8_t packet_size(const login_data& data);
        static void pack(const boost::intrusive_ptr<::kaminari::buffers::packet>& packet, const status_ex& data);
        static uint8_t packet_size(const status_ex& data);
        static uint8_t sizeof_status_ex();
        static void pack(const boost::intrusive_ptr<::kaminari::buffers::packet>& packet, const characters& data);
        static void pack(const boost::intrusive_ptr<::kaminari::buffers::packet>& packet, const character& data);
        static uint8_t packet_size(const character& data);
        static uint8_t packet_size(const characters& data);
        static bool unpack(::kaminari::buffers::packet_reader* packet, character_selection& data);
        static uint8_t packet_size(const character_selection& data);
        static uint8_t sizeof_character_selection();
        static void pack(const boost::intrusive_ptr<::kaminari::buffers::packet>& packet, const success& data);
        static uint8_t packet_size(const success& data);
        static uint8_t sizeof_success();
        static void pack(const boost::intrusive_ptr<::kaminari::buffers::packet>& packet, const complex& data);
        static void pack(const boost::intrusive_ptr<::kaminari::buffers::packet>& packet, const spawn_data& data);
        static uint8_t packet_size(const spawn_data& data);
        static uint8_t sizeof_spawn_data();
        static uint8_t packet_size(const complex& data);
        static bool unpack(::kaminari::buffers::packet_reader* packet, movement& data);
        static uint8_t packet_size(const movement& data);
        static uint8_t sizeof_movement();
        static void pack(const boost::intrusive_ptr<::kaminari::buffers::packet>& packet, const spawn& data);
        static uint8_t packet_size(const spawn& data);
        static uint8_t sizeof_spawn();
        static void pack(const boost::intrusive_ptr<::kaminari::buffers::packet>& packet, const despawn& data);
        static uint8_t packet_size(const despawn& data);
        static uint8_t sizeof_despawn();
        inline constexpr static uint8_t sizeof_int8();
        inline constexpr static uint8_t sizeof_int16();
        inline constexpr static uint8_t sizeof_int32();
        inline constexpr static uint8_t sizeof_int64();
        inline constexpr static uint8_t sizeof_uint8();
        inline constexpr static uint8_t sizeof_uint16();
        inline constexpr static uint8_t sizeof_uint32();
        inline constexpr static uint8_t sizeof_uint64();
        inline constexpr static uint8_t sizeof_float();
        inline constexpr static uint8_t sizeof_double();
        inline constexpr static uint8_t sizeof_bool();
        template <typename C>
        static bool handle_packet(::kaminari::buffers::packet_reader* packet, C* client);
    private:
        template <typename C>
        static bool handle_handshake(::kaminari::buffers::packet_reader* packet, C* client);
        template <typename C>
        static bool handle_login(::kaminari::buffers::packet_reader* packet, C* client);
        template <typename C>
        static bool handle_character_selected(::kaminari::buffers::packet_reader* packet, C* client);
        template <typename C>
        static bool handle_move(::kaminari::buffers::packet_reader* packet, C* client);
    };

    template <typename C>
    bool marshal::handle_handshake(::kaminari::buffers::packet_reader* packet, C* client)
    {
        if (!check_client_status(client, ingame_status::new_connection))
        {
            return handle_client_error(client, static_cast<::kumo::opcode>(packet->opcode()));
        }
        ::kumo::client_handshake data;
        if (!unpack(packet, data))
        {
            return false;
        }
        return on_handshake(client, data, packet->timestamp());
    }
    template <typename C>
    bool marshal::handle_login(::kaminari::buffers::packet_reader* packet, C* client)
    {
        if (!check_client_status(client, ingame_status::handshake_done))
        {
            return handle_client_error(client, static_cast<::kumo::opcode>(packet->opcode()));
        }
        ::kumo::login_data data;
        if (!unpack(packet, data))
        {
            return false;
        }
        return on_login(client, data, packet->timestamp());
    }
    template <typename C>
    bool marshal::handle_character_selected(::kaminari::buffers::packet_reader* packet, C* client)
    {
        if (!check_client_status(client, ingame_status::login_done))
        {
            return handle_client_error(client, static_cast<::kumo::opcode>(packet->opcode()));
        }
        ::kumo::character_selection data;
        if (!unpack(packet, data))
        {
            return false;
        }
        return on_character_selected(client, data, packet->timestamp());
    }
    template <typename C>
    bool marshal::handle_move(::kaminari::buffers::packet_reader* packet, C* client)
    {
        if (!check_client_status(client, ingame_status::in_world))
        {
            return handle_client_error(client, static_cast<::kumo::opcode>(packet->opcode()));
        }
        ::kumo::movement data;
        if (!unpack(packet, data))
        {
            return false;
        }
        return on_move(client, data, packet->timestamp());
    }
    inline constexpr uint8_t marshal::sizeof_int8()
    {
        return static_cast<uint8_t>(sizeof(int8_t));
    }
    inline constexpr uint8_t marshal::sizeof_int16()
    {
        return static_cast<uint8_t>(sizeof(int16_t));
    }
    inline constexpr uint8_t marshal::sizeof_int32()
    {
        return static_cast<uint8_t>(sizeof(int32_t));
    }
    inline constexpr uint8_t marshal::sizeof_int64()
    {
        return static_cast<uint8_t>(sizeof(int64_t));
    }
    inline constexpr uint8_t marshal::sizeof_uint8()
    {
        return static_cast<uint8_t>(sizeof(uint8_t));
    }
    inline constexpr uint8_t marshal::sizeof_uint16()
    {
        return static_cast<uint8_t>(sizeof(uint16_t));
    }
    inline constexpr uint8_t marshal::sizeof_uint32()
    {
        return static_cast<uint8_t>(sizeof(uint32_t));
    }
    inline constexpr uint8_t marshal::sizeof_uint64()
    {
        return static_cast<uint8_t>(sizeof(uint64_t));
    }
    inline constexpr uint8_t marshal::sizeof_float()
    {
        return static_cast<uint8_t>(sizeof(float));
    }
    inline constexpr uint8_t marshal::sizeof_double()
    {
        return static_cast<uint8_t>(sizeof(double));
    }
    inline constexpr uint8_t marshal::sizeof_bool()
    {
        return static_cast<uint8_t>(sizeof(bool));
    }
    template <typename C>
    bool marshal::handle_packet(::kaminari::buffers::packet_reader* packet, C* client)
    {
        switch (static_cast<::kumo::opcode>(packet->opcode()))
        {
            case opcode::handshake:
                return handle_handshake(packet, client);
            case opcode::login:
                return handle_login(packet, client);
            case opcode::character_selected:
                return handle_character_selected(packet, client);
            case opcode::move:
                return handle_move(packet, client);
            default:
                return false;
        }
    }
}
