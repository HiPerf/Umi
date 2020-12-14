#include <kumo/opcodes.hpp>
#include <kumo/marshal.hpp>
#include <kaminari/buffers/packet_reader.hpp>
namespace kumo
{
    bool marshal::unpack(::kaminari::packet_reader* packet, client_handshake& data)
    {
        if (packet->bytes_read() + sizeof_uint32() > packet->buffer_size())
        {
            return false;
        }
        data.version = packet->read<uint32_t>();
        return true;
    }
    uint8_t marshal::packet_size(const client_handshake& data)
    {
        (void)data;
        return sizeof(client_handshake);
    }
    uint8_t marshal::sizeof_client_handshake()
    {
        return sizeof(client_handshake);
    }
    void marshal::pack(const boost::intrusive_ptr<::kaminari::packet>& packet, const status& data)
    {
        *packet << data.success;
    }
    uint8_t marshal::packet_size(const status& data)
    {
        (void)data;
        return sizeof(status);
    }
    uint8_t marshal::sizeof_status()
    {
        return sizeof(status);
    }
    bool marshal::unpack(::kaminari::packet_reader* packet, login_data& data)
    {
        if (packet->bytes_read() + sizeof_uint8() > packet->buffer_size())
        {
            return false;
        }
        if (packet->bytes_read() + sizeof_uint8() + packet->peek<uint8_t>() > packet->buffer_size())
        {
            return false;
        }
        data.username = packet->read<std::string>();
        if (packet->bytes_read() + sizeof_uint64() > packet->buffer_size())
        {
            return false;
        }
        data.password0 = packet->read<uint64_t>();
        if (packet->bytes_read() + sizeof_uint64() > packet->buffer_size())
        {
            return false;
        }
        data.password1 = packet->read<uint64_t>();
        if (packet->bytes_read() + sizeof_uint64() > packet->buffer_size())
        {
            return false;
        }
        data.password2 = packet->read<uint64_t>();
        if (packet->bytes_read() + sizeof_uint64() > packet->buffer_size())
        {
            return false;
        }
        data.password3 = packet->read<uint64_t>();
        return true;
    }
    uint8_t marshal::packet_size(const login_data& data)
    {
        uint8_t size = 0;
        size += sizeof_uint8() + data.username.length();
        size += sizeof_uint64();
        size += sizeof_uint64();
        size += sizeof_uint64();
        size += sizeof_uint64();
        return size;
    }
    void marshal::pack(const boost::intrusive_ptr<::kaminari::packet>& packet, const status_ex& data)
    {
        *packet << data.code;
    }
    uint8_t marshal::packet_size(const status_ex& data)
    {
        (void)data;
        return sizeof(status_ex);
    }
    uint8_t marshal::sizeof_status_ex()
    {
        return sizeof(status_ex);
    }
    void marshal::pack(const boost::intrusive_ptr<::kaminari::packet>& packet, const characters& data)
    {
        *packet << static_cast<uint8_t>((data.list).size());
        for (const character& val : data.list)
        {
            pack(packet, val);
        }
    }
    void marshal::pack(const boost::intrusive_ptr<::kaminari::packet>& packet, const character& data)
    {
        *packet << data.name;
        *packet << data.level;
    }
    uint8_t marshal::packet_size(const character& data)
    {
        uint8_t size = 0;
        size += sizeof_uint8() + data.name.length();
        size += sizeof_uint16();
        return size;
    }
    uint8_t marshal::packet_size(const characters& data)
    {
        uint8_t size = 0;
        size += sizeof(uint8_t);
        for (const character& val : data.list)
        {
            size += packet_size(val);
        }
        return size;
    }
    bool marshal::unpack(::kaminari::packet_reader* packet, character_selection& data)
    {
        if (packet->bytes_read() + sizeof_uint8() > packet->buffer_size())
        {
            return false;
        }
        data.index = packet->read<uint8_t>();
        return true;
    }
    uint8_t marshal::packet_size(const character_selection& data)
    {
        (void)data;
        return sizeof(character_selection);
    }
    uint8_t marshal::sizeof_character_selection()
    {
        return sizeof(character_selection);
    }
    void marshal::pack(const boost::intrusive_ptr<::kaminari::packet>& packet, const complex& data)
    {
        *packet << static_cast<bool>(data.x);
        if (static_cast<bool>(data.x))
        {
            *packet << *data.x;
        }
        *packet << static_cast<uint8_t>((data.y).size());
        for (const spawn_data& val : data.y)
        {
            pack(packet, val);
        }
        *packet << data.z;
        *packet << static_cast<bool>(data.w);
        if (static_cast<bool>(data.w))
        {
            *packet << static_cast<uint8_t>((*data.w).size());
            for (const bool& val : *data.w)
            {
                *packet << val;
            }
        }
    }
    void marshal::pack(const boost::intrusive_ptr<::kaminari::packet>& packet, const spawn_data& data)
    {
        *packet << data.id;
        *packet << data.x;
        *packet << data.y;
    }
    uint8_t marshal::packet_size(const spawn_data& data)
    {
        (void)data;
        return sizeof(spawn_data);
    }
    uint8_t marshal::sizeof_spawn_data()
    {
        return sizeof(spawn_data);
    }
    uint8_t marshal::packet_size(const complex& data)
    {
        uint8_t size = 0;
        size += sizeof(bool);
        if (static_cast<bool>(data.x))
        {
            size += sizeof_uint32();
        }
        size += sizeof(uint8_t) + (data.y).size() * sizeof_spawn_data();
        size += sizeof_int32();
        size += sizeof(bool);
        if (static_cast<bool>(data.w))
        {
            size += sizeof(uint8_t) + (*data.w).size() * sizeof_bool();
        }
        return size;
    }
    bool marshal::unpack(::kaminari::packet_reader* packet, movement& data)
    {
        if (packet->bytes_read() + sizeof_int8() > packet->buffer_size())
        {
            return false;
        }
        data.direction = packet->read<int8_t>();
        return true;
    }
    uint8_t marshal::packet_size(const movement& data)
    {
        (void)data;
        return sizeof(movement);
    }
    uint8_t marshal::sizeof_movement()
    {
        return sizeof(movement);
    }
}
